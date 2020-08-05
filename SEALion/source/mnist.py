#######################
# DATA PRE-PROCESSING #
#######################

import time
import pandas as pd
import numpy as np
import sealion as sl
import sealion.heras.datasets.mnist as mnist

# Load MNIST dataset. These are just regular numpy arrays.
(x_train, y_train), (x_test, y_test) = mnist.load_data()

# We discretize into 4 bins (e.g. 256/64). Double slash means it rounds
# down to integers.
x_train = (np.reshape(x_train, [-1, 784]) + 1.0) // 64
x_test = (np.reshape(x_test, [-1, 784]) + 1.0) // 64

# Convert the labels to one-hot encoded vectors.
y_train = sl.heras.utils.to_categorical(y_train, 10)
y_test = sl.heras.utils.to_categorical(y_test, 10)

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


def delta_ms(t0, t1):
    return round(1000 * abs(t0 - t1))


####################
# MODEL DEFINITION #
####################

from sealion.heras.models import Sequential
from sealion.heras.layers import Dense, Activation

model = Sequential()
model.add(Dense(units=30, input_dim=784,
                input_saturation=4,
                saturation=2 ** 4))
model.add(Activation())
model.add(Dense(units=10, saturation=2 ** 4))

model.compile(loss='categorical_crossentropy',
              optimizer='adam',
              metrics=['accuracy'])

for run in range(10):
    cur_times = times

    ############
    # TRAINING #
    ############
    t0 = time.perf_counter()
    model.fit(x_train, y_train, epochs=1,
              batch_size=64)
    t1 = time.perf_counter()
    cur_times['t_training'] = delta_ms(t0, t1)

    ################################
    # PREDICTING ON ENCRYPTED DATA #
    ################################

    # Generate a new keypair.
    t0 = time.perf_counter()
    params = model.params
    pk, sk = sl.Keypair(params)
    t1 = time.perf_counter()
    cur_times['t_keygen'] = delta_ms(t0, t1)

    # Encrypt the test set.
    t0 = time.perf_counter()
    e_x_test = pk.encrypt(x_test)
    t1 = time.perf_counter()
    cur_times['t_input_encryption'] = delta_ms(t0, t1)

    # Generate encrypted predictions for the encrypted test set.
    t0 = time.perf_counter()
    e_p_test, metrics = model.predict(e_x_test, encrypted=True)
    t1 = time.perf_counter()
    cur_times['t_computation'] = delta_ms(t0, t1)

    # Decrypt the encrypted predictions.
    t0 = time.perf_counter()
    p_test = sk.decrypt(e_p_test)
    p_test = np.argmax(p_test, axis=-1)
    t1 = time.perf_counter()
    cur_times['t_decryption'] = delta_ms(t0, t1)

    # This call will take care of the encryption of x_test, decryption of
    # e_y_test and compute the accuracy and some other metrics.
    metrics = model.evaluate(x_test, y_test, encrypted=True)

    print("Accuracy: {}%, latency: {}s, "
          "throughput: {}/h".format(
        metrics.accuracy * 100,
        metrics.latency,
        metrics.throughput * 3600))

    cur_times['test_accuracy'] = metrics.accuracy
    print(cur_times)
    all_times.append(cur_times)

# Output the benchmarking results
df = pd.DataFrame(all_times)
df.to_csv('sealion_nn.csv', index=False)
