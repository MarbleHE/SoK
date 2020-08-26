# ******************************************************************************
# Copyright 2018-2020 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# *****************************************************************************

import numpy as np
import tensorflow as tf
from tensorflow.keras.layers import (
    Dense,
    Activation
)


def mnist_mlp_model(input):
    def square_activation(x):
        return x * x

    # Using Keras model API with Flatten results in split ngraph at Flatten() or Reshape() op.
    # Use tf.reshape instead
    known_shape = input.get_shape()[1:]
    size = np.prod(known_shape)
    print('size', size)
    y = tf.reshape(input, [-1, size])
    # y = Flatten()(input)
    y = Dense(input_shape=[1, 784], units=30, use_bias=True)(y)
    y = Activation(square_activation)(y)
    y = Dense(units=10, use_bias=True, name="output")(y)
    known_shape = y.get_shape()[1:]
    size = np.prod(known_shape)
    print('size', size)
    return y
