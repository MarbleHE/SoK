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

import ngraph_bridge
import argparse
import numpy as np
import tensorflow as tf
from tensorflow.core.protobuf import rewriter_config_pb2


def str2bool(v):
    if isinstance(v, bool):
        return v
    if v.lower() in ("on", "yes", "true", "t", "y", "1"):
        return True
    elif v.lower() in ("off", "no", "false", "f", "n", "0"):
        return False
    else:
        raise argparse.ArgumentTypeError("Boolean value expected.")


def server_config(tensor_param_name):
    rewriter_options = rewriter_config_pb2.RewriterConfig()
    rewriter_options.meta_optimizer_iterations = rewriter_config_pb2.RewriterConfig.ONE
    rewriter_options.min_graph_nodes = -1
    server_config = rewriter_options.custom_optimizers.add()
    server_config.name = "ngraph-optimizer"
    server_config.parameter_map["ngraph_backend"].s = "HE_SEAL".encode()
    server_config.parameter_map["device_id"].s = b""
    server_config.parameter_map[
        "encryption_parameters"].s = "/home/he-transformer/configs/he_seal_ckks_config_N11_L1.json".encode()
    server_config.parameter_map["enable_client"].s = (str(True)).encode()
    #if FLAGS.enable_client:
    server_config.parameter_map[tensor_param_name].s = b"client_input"

    config = tf.compat.v1.ConfigProto()
    config.MergeFrom(
        tf.compat.v1.ConfigProto(
            graph_options=tf.compat.v1.GraphOptions(
                rewrite_options=rewriter_options)))

    return config


def main():

    a = tf.constant(np.array([[1, 2, 3, 4]]), dtype=np.float32)
    b = tf.compat.v1.placeholder(
        tf.float32, shape=(1, 4), name="client_parameter_name")
    c = tf.compat.v1.placeholder(tf.float32, shape=(1, 4))
    f = c * (a + b)

    # Create config to load parameter b from client
    config = server_config(b.name)
    print("config", config)

    with tf.compat.v1.Session(config=config) as sess:
        f_val = sess.run(f, feed_dict={b: np.ones((1, 4)), c: np.ones((1, 4))})
        print("Result: ", f_val)


if __name__ == "__main__":
    main()
