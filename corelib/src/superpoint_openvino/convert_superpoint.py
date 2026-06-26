#!/usr/bin/env python3
import argparse
import os

import torch
import openvino as ov

INPUT_SHAPE = [1, 1, 480, 848]  # NPU precisa de shape fixo


def convert_model(input_path: str, output_path: str):
    model = torch.jit.load(input_path, map_location="cpu")
    model.eval()

    dummy_input = torch.randn(*INPUT_SHAPE)

    print("Convertendo TorchScript para OpenVINO IR com shape fixo...")

    ov_model = ov.convert_model(
        model,
        example_input=dummy_input
    )

    ov_model.reshape({ov_model.inputs[0]: INPUT_SHAPE})

    print("Salvando modelo em FP16 para NPU...")
    output_dir = os.path.dirname(output_path)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)
    ov.save_model(
        ov_model,
        output_path,
        compress_to_fp16=True
    )

    print("OpenVINO IR exportado com sucesso!")

    print("Verificando shape final:")
    core = ov.Core()
    check_model = core.read_model(output_path)

    for inp in check_model.inputs:
        print(inp.any_name if inp.names else "input", inp.partial_shape)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert SuperPoint TorchScript model to OpenVINO IR"
    )
    parser.add_argument("--input", required=True, help="Path to input TorchScript (.pt) model")
    parser.add_argument("--output", required=True, help="Path to output OpenVINO IR (.xml) model")
    args = parser.parse_args()
    print(f"Converting model from {args.input} to {args.output}")

    convert_model(args.input, args.output)
