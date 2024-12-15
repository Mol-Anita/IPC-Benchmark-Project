import os

def generate_random_file(filename, size_in_kb):
    size_in_bytes = size_in_kb * 1024
    with open(filename, "wb") as f:
        f.write(os.urandom(size_in_bytes))

generate_random_file("1kb.bin", 1)
generate_random_file("1kb.bin", 8)
generate_random_file("16kb.bin", 16)
generate_random_file("32kb.bin", 32)

