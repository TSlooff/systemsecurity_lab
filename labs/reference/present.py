sbox = [0xc, 0x5, 0x6, 0xb, 0x9, 0x0, 0xa, 0xd, 0x3, 0xe, 0xf, 0x8, 0x4, 0x7, 0x1, 0x2]
sbox_inv = [sbox.index(x) for x in range(16)]

permutations = [
    0, 16, 32, 48, 1, 17, 33, 49, 2, 18, 34, 50, 3, 19, 35, 51,
    4, 20, 36, 52, 5, 21, 37, 53, 6, 22, 38, 54, 7, 23, 39, 55,
    8, 24, 40, 56, 9, 25, 41, 57, 10, 26, 42, 58, 11, 27, 43, 59,
    12, 28, 44, 60, 13, 29, 45, 61, 14, 30, 46, 62, 15, 31, 47, 63
]

permutations_inv = [permutations.index(x) for x in range(64)]

def left_rotate(n, d):
    return ((n << d)|(n >> (80 - d))) & 0xffffffffffffffffffff

def generate_round_keys(key: int):
    round_keys = []
    round_keys.append(key >> 16)
    for i in range(1, 32):
        # 1. left rotate key
        key = left_rotate(key, 61) 
        # 2. set left-most 4 bits to sbox output of left-most 4 bits
        sbox_input = (key >> 76) & 0xf # take left 4 bits
        key &= (0xffffffffffffffffffff >> 4) # 0 out left 4 bits
        key |= (sbox[sbox_input] << 76) # set the left 4 bits to output of sbox
        # 3. xor bits 19-15 with round_counter
        key ^= (i & 0b11111) << 15
        round_keys.append(key >> 16)
        
    return round_keys

def add_round_key(state:int, k_i:int):
    return state ^ k_i

def sbox_layer(state:int):
    output = 0x0
    for i in range(16):
        output |= sbox[((state >> (4*i)) & 0xf)] << (i*4)
    return output
    
def permutation_layer(state:int):
    output = 0x0
    for i in range(64):
        output |= (((state >> (63 - i)) & 0x1) << (63-permutations[i]))
    return output
    
def encrypt(plaintext:int, key:int):
    roundkeys = generate_round_keys(key)
    state = plaintext
    for i in range(31):
        state = add_round_key(state, roundkeys[i])
        state = sbox_layer(state)
        state = permutation_layer(state)
    state = add_round_key(state, roundkeys[31])
    return state

def sbox_layer_inv(state:int):
    output = 0x0
    for i in range(16):
        output |= sbox_inv[((state >> (4*i)) & 0xf)] << (i*4)
    return output
    
def permutation_layer_inv(state:int):
    output = 0x0
    for i in range(64):
        output |= (((state >> (63 - i)) & 0x1) << (63-permutations_inv[i]))
    return output
    
def decrypt(ciphertext:int, key:int):
    roundkeys = generate_round_keys(key)
    state = ciphertext
    for i in range(31, 0, -1):
        state = add_round_key(state, roundkeys[i])
        state = permutation_layer_inv(state)
        state = sbox_layer_inv(state)
    state = add_round_key(state, roundkeys[0])
    return state