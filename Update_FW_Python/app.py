import serial

START = 0xAC
MAJOR = 1
MINOR = 0
BUILD = 0
NEW_FW = 1

expected_response = bytearray([0x55])
COMPORT ='COM3'
fwFile = 'Application.bin'

def crc16(data: bytearray, offset: int, length: int) -> int:
    
    if data is None or offset < 0 or offset >= len(data) or (offset + length) > len(data):
        return 0 

    crc = 0xFFFF  
    for i in range(offset, offset + length):
        crc ^= data[i] << 8 
        for j in range(8):  
            if crc & 0x8000: 
                crc = (crc << 1) ^ 0x1021  
            else:
                crc <<= 1 

    return crc & 0xFFFF 


def read_binary_in_chunks(file_path, chunk_size = 64):
    with open(file_path, 'rb') as file:
        while chunk := file.read(chunk_size): 
            yield chunk 


#  AC - len - NEW_FW - major - minor - build - app len high - app len low - crc high - crc low  
def pack_header(length):
    result = []
    buff = []
   
    result.append(START.to_bytes(1, byteorder='little')) 
    buff.append((6).to_bytes(1, byteorder='little'))  
    buff.append(NEW_FW.to_bytes(1, byteorder='little')) 
    buff.append(MAJOR.to_bytes(1, byteorder='little')) 
    buff.append(MINOR.to_bytes(1, byteorder='little')) 
    buff.append(BUILD.to_bytes(1, byteorder='little')) 
    buff.append(length.to_bytes(2, byteorder='big')) 
    buff = bytearray().join(buff)  
    result = bytearray().join(result)  
    print(len(result))
    crc = crc16(buff, 0, len(buff))  
    result = result + buff + crc.to_bytes(2, byteorder='big')
    return result  
 
def pack_data(buff, length, CMD):
    result = []
    buffTemp = []
    result.append(START.to_bytes(1, byteorder='little')) 
    buffTemp.append((length+1).to_bytes(1, byteorder='little')) 
    buffTemp.append(CMD.to_bytes(1, byteorder='little')) 
    buffTemp.append(buff) 
    buffTemp = bytearray().join(buffTemp) 

    result = bytearray().join(result)  
    print(len(result))
    crc = crc16(buffTemp, 0, len(buffTemp)) 
    result = result + buffTemp + crc.to_bytes(2, byteorder='big')  # Thêm CRC16 vào cuối
    return result

ser = serial.Serial(
    port=COMPORT,  
    baudrate=115200, 
    timeout=1  
)

if ser.is_open:
    print("Open Serial port successfully!")
else:
    print("Can't open serial!")

#wait reset signal
print("wait signal from stm32")
while True:
        if ser.in_waiting >= 1: 
            response = ser.read(1)  
            if response[0] == 0xAC:
                print(f"Received response: 0x{response.hex()}")
                break 
            print(f"Received response: 0x{response.hex()}")

#send data about firmware 
with open(fwFile, "rb") as f:
    f.seek(0, 2)  
    file_size = f.tell()  
print(f"Size of file: {file_size} bytes")
buff = pack_header(file_size)
print(f"Sent: {buff.hex()}")
ser.write(buff)
#wait response
offset = 0x0000
print("Wait signal from stm32")
while True:
        if ser.in_waiting >= 1:  
            response = ser.read(1) 
            if response[0] == 0x55:
                print(f"Received response: 0x{response.hex()}")
                break 
            print(f"Received response: {response.hex()}")
  
#start transfer firmware
for chunk in read_binary_in_chunks(fwFile):
    # print(chunk) 
    #read file
    length = len(chunk)
    if length <= 0:
        break; 
    buff = pack_data(chunk, length, 2) 
    ser.write(buff)
    # ser.write(testbuff)
    print(len(buff))
    print(f"Sent: {buff.hex()}")
    response  = bytearray()
    while True:
        if ser.in_waiting >= 1: 
            response = ser.read(1) 
            break  
    if response == expected_response:
        print(f"Received expected response: {response.hex()}")
        offset += length
    else:
        print(f"Unexpected response: {response.hex()}")

print("Transfer completed"); 
ser.close()