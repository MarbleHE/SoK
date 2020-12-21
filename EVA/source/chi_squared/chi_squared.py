# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

from eva import EvaProgram, Input, Output, evaluate, save, load
from eva.ckks import CKKSCompiler
from eva.seal import generate_keys
from eva.metric import valuation_mse
import numpy as np
from google.protobuf.json_format import MessageToJson
from google.protobuf import text_format
import known_type_pb2
import eva_pb2
import vec_lang_pb2

#################################################
print('Compile time')

chi_squared = EvaProgram('Chi Squared', vec_size=1)
with chi_squared:
    n0 = Input('n0')
    n1 = Input('n1')
    n2 = Input('n2')

    common_term = 2 * n0 + n1

    Output('alpha', (4 * n0 * n2 - n1 ** 2) ** 2)
    Output('beta1', (2 * common_term) ** 2)
    Output('beta2', common_term * (2 * n2 + n1))
    Output('beta3', 2 * (2 * n2 + n1) ** 2)

    chi_squared.set_output_ranges(60)
    chi_squared.set_input_scales(60)

    compiler = CKKSCompiler()
    chi_squared, params, signature = compiler.compile(chi_squared)

    save(chi_squared, 'chi_squared.eva')
    save(params, 'chi_squared.evaparams')
    save(signature, 'chi_squared.evasignature')

    # Print IR representation
    with open('chi_squared.eva', 'rb') as f, open('chi_squared.txt', 'w') as g:
        read_kt = known_type_pb2.KnownType()
        read_kt.ParseFromString(f.read())
        read_eva = eva_pb2.Program()
        read_eva.ParseFromString(read_kt.contents.value)
        g.write(str(read_eva))

    #################################################
    print('Key generation time')

    params = load('chi_squared.evaparams')

    public_ctx, secret_ctx = generate_keys(params)

    save(public_ctx, 'chi_squared.sealpublic')
    save(secret_ctx, 'chi_squared.sealsecret')

    #################################################
    print('Runtime on client')

    signature = load('chi_squared.evasignature')
    public_ctx = load('chi_squared.sealpublic')

    n0_val = 2
    n1_val = 7
    n2_val = 9
    inputs = {
        'n0': [n0_val],
        'n1': [n1_val],
        'n2': [n2_val]
    }
    encInputs = public_ctx.encrypt(inputs, signature)

    save(encInputs, 'chi_squared_inputs.sealvals')

    #################################################
    print('Runtime on server')

    chi_squared = load('chi_squared.eva')
    public_ctx = load('chi_squared.sealpublic')
    encInputs = load('chi_squared_inputs.sealvals')

    encOutputs = public_ctx.execute(chi_squared, encInputs)

    save(encOutputs, 'chi_squared_outputs.sealvals')

    #################################################
    print('Back on client')

    secret_ctx = load('chi_squared.sealsecret')
    encOutputs = load('chi_squared_outputs.sealvals')

    outputs = secret_ctx.decrypt(encOutputs, signature)

    alpha_expected = (4 * n0_val * n2_val - n1_val ** 2) ** 2
    beta1_expected = 2 * (2 * n0_val + n1_val) ** 2
    beta2_expected = (2 * n0_val + n1_val) * (2 * n2_val + n1_val)
    beta3_expected = 2 * (2 * n2_val + n1_val) ** 2
    reference = {'alpha': [alpha_expected],
                 'beta1': [beta1_expected],
                 'beta2': [beta2_expected],
                 'beta3': [beta3_expected]}
    print('Expected', reference)
    print('Got', outputs)
    print('MSE', valuation_mse(outputs, reference))
