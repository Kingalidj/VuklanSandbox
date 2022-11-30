import sys

with open(sys.argv[0], 'rb') as file:
    while (byte := file.read(1)):
        print(f"0x{byte.hex()},")


