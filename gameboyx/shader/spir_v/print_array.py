# this script is just used to output the spir-v bytecode as a vector of bytes -> copy paste into source code
# currently cuts off elements when number of hex digits mod 32 != 0, but better than typing out the whole hex file

import sys

if len(sys.argv) == 2:
    file = open(sys.argv[1], 'rb')
    hexdata = file.read().hex()
    print("const vector<u8> placeholder = {", end="")
    
    output = ""
    lines = int(len(hexdata) / 32)
    rest = len(hexdata) % 32
    elements = 16;
    for n in range(lines):
        print("\n\t", end="")
        for m in range(elements):
            if n == lines - 1 and m == elements - 1 and rest == 0:
                output = "0x{}{}"
            else:
                output = "0x{}{}, "
            
            print(output.format(hexdata[n * 32 + (m * 2)], hexdata[n * 32 + (m * 2) + 1]), end="") 
    
    if rest > 0:   
        print("\n\t", end="")  
        for x in range(len(hexdata) - rest, len(hexdata) - 2, 2):
            print("0x{}{}, ".format(hexdata[x], hexdata[x + 1]), end="")
        print("0x{}{}".format(hexdata[len(hexdata) - 2], hexdata[len(hexdata) - 1]), end="")
    print("\n};")
else:
    print("Wrong number of arguments")