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
from tensorflow.keras.layers import Layer, Input, Dense
from tensorflow.keras.optimizers import SGD
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


class PolyAct(Layer):
    def __init__(self, **kwargs):
        super(PolyAct, self).__init__(**kwargs)

    def build(self, input_shape):
        self.coeff = self.add_weight('coeff', shape=(2, 1), initializer="random_normal", trainable=True, )

    def call(self, inputs):
        return self.coeff[1] * K.square(inputs) + self.coeff[0] * inputs

    def compute_output_shape(self, input_shape):
        return input_shape


####################
# MODEL DEFINITION #
####################
def mnist_mlp_model(input):
    # Using Keras model API with Flatten results in split ngraph at Flatten() or Reshape() op.
    # Use tf.reshape instead
    known_shape = input.get_shape()[1:]
    size = np.prod(known_shape)
    print('size', size)
    y = tf.reshape(input, [-1, size])
    # y = Flatten()(input)
    y = Dense(input_shape=[1, 784], units=30, use_bias=True)(y)
    y = PolyAct()(y)
    y = Dense(units=10, use_bias=True, name="output")(y)
    known_shape = y.get_shape()[1:]
    size = np.prod(known_shape)
    print('size', size)
    return y


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
    y = mnist_mlp_model(x)

    mlp_model = Model(inputs=x, outputs=y)
    print(mlp_model.summary())

    def loss(labels, logits):
        return categorical_crossentropy(labels, logits, from_logits=True)

    optimizer = SGD(learning_rate=0.008, momentum=0.9)
    mlp_model.compile(optimizer=optimizer, loss=loss, metrics=["accuracy"])

    t0 = time.perf_counter()
    mlp_model.fit(
        x_train,
        y_train,
        epochs=10,
        batch_size=128,
        validation_data=(x_test, y_test),
        verbose=1)

    t1 = time.perf_counter()
    cur_times['t_training'] = delta_ms(t0, t1)

    test_loss, test_acc = mlp_model.evaluate(x_test, y_test, verbose=1)
    print("\nTest accuracy:", test_acc)

    save_model(
        tf.compat.v1.keras.backend.get_session(),
        ["output/BiasAdd"],
        "./models",
        "mlp",
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
    tf.import_graph_def(load_pb_file("models/mlp.pb"))

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
    output_filename = "nn-mlp-learned.csv"
    if 'OUTPUT_FILENAME' in os.environ:
        output_filename = os.environ['OUTPUT_FILENAME']
    df.to_csv(output_filename, index=False)


if __name__ == "__main__":
    main()
