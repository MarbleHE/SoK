# ==============================================================================
#  Copyright 2018-2020 Intel Corporation
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
# ==============================================================================
"""An MNIST classifier using convolutional layers and relu activations. """

import time
import os
import copy
import numpy as np
import pandas as pd
import tensorflow as tf
from tensorflow.keras import backend as K
from tensorflow.keras.models import Model
from tensorflow.keras.layers import Input, Dense, Conv2D, Activation, AveragePooling2D, Flatten, Reshape, Layer
from tensorflow.keras.optimizers import Adam
from tensorflow.keras.losses import categorical_crossentropy
import ngraph_bridge

# Add parent directory to path
from mnist_util import (
    load_mnist_data,
    server_config,
    save_model,
    load_pb_file,
    print_nodes
)

####################
# BENCHMARKING     #
####################
times = {
    't_training': [],
    't_keygen': [],
    't_input_encryption': [],
    't_computation': [],
    't_decryption': [],
    'test_accuracy': []
}
all_times = []
cur_times = []


def delta_ms(t0, t1):
    return round(1000 * abs(t0 - t1))


####################
# MODEL DEFINITION #
####################
class PolyAct(Layer):
    def __init__(self, **kwargs):
        super(PolyAct, self).__init__(**kwargs)
        self.coeff = []

    def build(self, input_shape):
        self.coeff = self.add_weight('coeff', shape=(2, 1), initializer="random_normal", trainable=True, )

    def call(self, inputs):
        return self.coeff[1] * K.square(inputs) + self.coeff[0] * inputs

    def get_weights(self):
        return self.coeff

    def set_weights(self, weights):
        self.coeff = weights

    def compute_output_shape(self, input_shape):
        return input_shape


def cryptonets_model(input):
    y = Conv2D(
        filters=5,
        kernel_size=(5, 5),
        strides=(2, 2),
        padding="same",
        use_bias=True,
        input_shape=(28, 28, 1),
        name="conv2d_1",
    )(input)
    y = PolyAct(name='act_1')(y)

    y = AveragePooling2D(pool_size=(3, 3), strides=(1, 1), padding="same")(y)
    y = Conv2D(
        filters=50,
        kernel_size=(5, 5),
        strides=(2, 2),
        padding="same",
        use_bias=True,
        name="conv2d_2",
    )(y)
    y = AveragePooling2D(pool_size=(3, 3), strides=(1, 1), padding="same")(y)
    y = Flatten()(y)
    y = Dense(100, use_bias=True, name="fc_1")(y)
    y = PolyAct(name='act_2')(y)
    y = Dense(10, use_bias=True, name="fc_2")(y)

    return y


def cryptonets_model_squashed(input, conv1_weights, squashed_weights, fc2_weights, act1_weights, act2_weights):
    y = Conv2D(
        filters=5,
        kernel_size=(5, 5),
        strides=(2, 2),
        padding="same",
        use_bias=True,
        kernel_initializer=tf.compat.v1.constant_initializer(conv1_weights[0]),
        bias_initializer=tf.compat.v1.constant_initializer(conv1_weights[1]),
        input_shape=(28, 28, 1),
        trainable=False,
        name="convd1_1",
    )(input)
    act_1 = PolyAct()
    act_1.set_weights(act1_weights)
    y = act_1(y)

    # Using Keras model API with Flatten results in split ngraph at Flatten() or Reshape() op.
    # Use tf.reshape instead
    y = tf.reshape(y, [-1, 5 * 14 * 14])
    y = Dense(
        100,
        use_bias=True,
        name="squash_fc_1",
        trainable=False,
        kernel_initializer=tf.compat.v1.constant_initializer(
            squashed_weights[0]),
        bias_initializer=tf.compat.v1.constant_initializer(squashed_weights[1]),
    )(y)
    act_2 = PolyAct()
    act_2.set_weights(act2_weights)
    y = act_2(y)

    y = Dense(
        10,
        use_bias=True,
        trainable=False,
        kernel_initializer=tf.compat.v1.constant_initializer(fc2_weights[0]),
        bias_initializer=tf.compat.v1.constant_initializer(fc2_weights[1]),
        name="output",
    )(y)

    return y


# Squash linear layers and return squashed weights
def squash_layers(cryptonets_model, sess):
    layers = cryptonets_model.layers
    layer_names = [layer.name for layer in layers]
    conv1_weights = layers[layer_names.index('conv2d_1')].get_weights()
    conv2_weights = layers[layer_names.index('conv2d_2')].get_weights()
    fc1_weights = layers[layer_names.index('fc_1')].get_weights()
    fc2_weights = layers[layer_names.index('fc_2')].get_weights()
    act1_weights = layers[layer_names.index('act_1')].get_weights()
    act2_weights = layers[layer_names.index('act_2')].get_weights()

    # Get squashed weight
    y = Input(shape=(14 * 14 * 5,), name="squashed_input")
    y = Reshape((14, 14, 5))(y)
    y = AveragePooling2D(pool_size=(3, 3), strides=(1, 1), padding="same")(y)
    y = Conv2D(
        filters=50,
        kernel_size=(5, 5),
        strides=(2, 2),
        padding="same",
        use_bias=True,
        trainable=False,
        kernel_initializer=tf.compat.v1.constant_initializer(conv2_weights[0]),
        bias_initializer=tf.compat.v1.constant_initializer(conv2_weights[1]),
        name="conv2d_test",
    )(y)
    y = AveragePooling2D(pool_size=(3, 3), strides=(1, 1), padding="same")(y)
    y = Flatten()(y)
    y = Dense(
        100,
        use_bias=True,
        name="fc_1",
        kernel_initializer=tf.compat.v1.constant_initializer(fc1_weights[0]),
        bias_initializer=tf.compat.v1.constant_initializer(fc1_weights[1]))(y)

    sess.run(tf.compat.v1.global_variables_initializer())

    # Pass 0 to get bias
    squashed_bias = y.eval(
        session=sess,
        feed_dict={
            "squashed_input:0": np.zeros((1, 14 * 14 * 5))
        })
    squashed_bias_plus_weights = y.eval(
        session=sess, feed_dict={
            "squashed_input:0": np.eye(14 * 14 * 5)
        })
    squashed_weights = squashed_bias_plus_weights - squashed_bias

    print("squashed layers")

    # Sanity check
    x_in = np.random.rand(100, 14 * 14 * 5)
    network_out = y.eval(session=sess, feed_dict={"squashed_input:0": x_in})
    linear_out = x_in.dot(squashed_weights) + squashed_bias
    assert np.max(np.abs(linear_out - network_out)) < 1e-3

    return (conv1_weights, (squashed_weights, squashed_bias), fc1_weights,
            fc2_weights, act1_weights, act2_weights)


####################
# TRAINING         #
####################
def train_model():
    (x_train, y_train, x_test, y_test) = load_mnist_data()

    x = Input(
        shape=(
            28,
            28,
            1,
        ), name="input")

    y = cryptonets_model(x)
    cryptonets_model_var = Model(inputs=x, outputs=y)
    print(cryptonets_model_var.summary())

    def loss(labels, logits):
        return categorical_crossentropy(
            labels, logits, from_logits=True)

    optimizer = Adam()
    cryptonets_model_var.compile(
        optimizer=optimizer, loss=loss, metrics=["accuracy"])

    t0 = time.perf_counter()
    cryptonets_model_var.fit(
        x_train,
        y_train,
        epochs=10,
        batch_size=128,
        validation_data=(x_test, y_test),
        verbose=1)
    t1 = time.perf_counter()
    cur_times['t_training'] = delta_ms(t0, t1)

    test_loss, test_acc = cryptonets_model_var.evaluate(x_test, y_test, verbose=1)
    print("Test accuracy:", test_acc)

    # Squash weights and save model
    weights = squash_layers(cryptonets_model_var,
                            tf.compat.v1.keras.backend.get_session())
    (conv1_weights, squashed_weights, fc1_weights, fc2_weights, act1_weights, act2_weights) = weights[0:6]

    tf.reset_default_graph()
    sess = tf.compat.v1.Session()

    x = Input(
        shape=(
            28,
            28,
            1,
        ), name="input")
    y = cryptonets_model_squashed(x, conv1_weights, squashed_weights, fc2_weights, act1_weights, act2_weights)
    sess.run(tf.compat.v1.global_variables_initializer())
    save_model(
        sess,
        ["output/BiasAdd"],
        "./models",
        "cryptonets",
    )

    # If we want to have training and testing in the same run, we must clean up after training
    tf.keras.backend.clear_session
    tf.reset_default_graph()


####################
# TESTING          #
####################
def test_network():
    batch_size = 4000
    # Load MNIST data (for test set)
    (x_train, y_train, x_test, y_test) = load_mnist_data(start_batch=0, batch_size=batch_size)

    # Load saved model
    tf.import_graph_def(load_pb_file("models/cryptonets.pb"))

    print("loaded model")
    print_nodes()

    # Get input / output tensors
    x_input = tf.compat.v1.get_default_graph().get_tensor_by_name("import/input:0")
    y_output = tf.compat.v1.get_default_graph().get_tensor_by_name("import/output/BiasAdd:0")

    # Create configuration to encrypt input
    config = server_config(x_input.name)
    with tf.compat.v1.Session(config=config) as sess:
        sess.run(tf.compat.v1.global_variables_initializer())
        start_time = time.time()
        t0 = time.perf_counter()
        y_hat = y_output.eval(feed_dict={x_input: x_test})
        t1 = time.perf_counter()
        cur_times['t_computation'] = delta_ms(t0, t1)
        elasped_time = time.time() - start_time
        print("total time(s)", np.round(elasped_time, 3))

    y_test_label = np.argmax(y_test, 1)

    if batch_size < 60:
        print("y_hat", np.round(y_hat, 2))

    y_pred = np.argmax(y_hat, 1)
    correct_prediction = np.equal(y_pred, y_test_label)
    error_count = np.size(correct_prediction) - np.sum(correct_prediction)
    test_accuracy = np.mean(correct_prediction)

    print("Error count", error_count, "of", batch_size, "elements.")
    print("Accuracy: %g " % test_accuracy)
    cur_times['test_accuracy'] = test_accuracy


def main():
    num_runs = int(os.getenv("NUM_RUNS")) if os.getenv("NUM_RUNS") is not None else 1
    for run in range(num_runs):
        global cur_times
        cur_times = copy.copy(times)

        train_model()
        ################################
        # PREDICTING ON ENCRYPTED DATA #
        ################################
        cur_times['t_keygen'] = 0  # TODO: FIND KEYGEN -seems to be buried pretty deep
        cur_times['t_input_encryption'] = 0  # TODO: FIND ENCRYPTION?
        cur_times['t_decryption'] = 0  # TODO: FIND DECRYPTION - seems to be buried pretty deep, too

        test_network()

        print(cur_times)
        all_times.append(cur_times)

    # Output the benchmarking results
    df = pd.DataFrame(all_times)
    output_filename = "nn-lenet5-learned.csv"
    if 'OUTPUT_FILENAME' in os.environ:
        output_filename = os.environ['OUTPUT_FILENAME']
    df.to_csv(output_filename, index=False)


if __name__ == "__main__":
    main()
