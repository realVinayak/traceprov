import math


def add_underscores_numbers(input_number: int, chunk_by: int = 3):
    # take something like 86243234 and make it 86_243_234 (for chunk_by = 3)
    # starts from the right.
    assert isinstance(input_number, int), "got unexpected type"
    input_number_str = str(input_number)
    remainder = len(input_number_str) % chunk_by
    first_chunk = input_number_str[:remainder]
    remaining = input_number_str[remainder:]
    chunks = []
    while len(remaining) > 0:
        chunk = remaining[0:chunk_by]
        remaining = remaining[chunk_by:]
        chunks.append(chunk)
    if first_chunk:
        chunks = [first_chunk, *chunks]
    return "_".join(chunks)


def get_filter_group(num_groups, selectivity, mode):
    multiplier = -1 if mode == "pre" else 1
    print(num_groups * selectivity, "num_gs")
    return int(selectivity * num_groups / 100) * multiplier


if __name__ == "__main__":
    print(add_underscores_numbers(86243234))
    print(add_underscores_numbers(1_000_000))
    print(add_underscores_numbers(10_000_000))
    print(add_underscores_numbers(100_000_000))
