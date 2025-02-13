import serial

START = 0xAC
MAJOR = 1
MINOR =0
BUILD =0
NEW_FW =1
expected_response = bytearray([0x55])
COMPORT ='COM3'
fwFile = 'Application.bin'
def crc16(data: bytearray, offset: int, length: int) -> int:
    # Kiểm tra các điều kiện hợp lệ của dữ liệu và offset
    if data is None or offset < 0 or offset >= len(data) or (offset + length) > len(data):
        return 0  # Trả về CRC là 0 nếu điều kiện không hợp lệ

    crc = 0xFFFF  # Giá trị khởi tạo CRC16
    for i in range(offset, offset + length):  # Duyệt qua mảng dữ liệu từ offset đến offset + length
        crc ^= data[i] << 8  # Đưa byte vào vị trí cao nhất của CRC
        for j in range(8):  # Vòng lặp 8 lần để xử lý từng bit
            if crc & 0x8000:  # Kiểm tra bit cao nhất (MSB)
                crc = (crc << 1) ^ 0x1021  # Dịch trái và XOR với 0x1021 nếu bit cao nhất là 1
            else:
                crc <<= 1  # Dịch trái nếu bit cao nhất là 0

    return crc & 0xFFFF  # Trả về CRC 16-bit cuối cùng


def read_binary_in_chunks(file_path, chunk_size = 64):
    with open(file_path, 'rb') as file:
        while chunk := file.read(chunk_size):  # Đọc từng khối byte
            yield chunk  # Trả về khối byte mỗi lần

#length of data
#command 
#buffer data
#  AC - len - NEW_FW - major - minor - build - app len high - app len low - crc high - crc low  
def pack_header(length):
    result = []
    buff = []
    # Chuyển PID, MID, CMD thành byte và thêm vào result
    result.append(START.to_bytes(1, byteorder='little'))  # Chuyển PID thành byte (nếu PID là số nguyên)
    buff.append((6).to_bytes(1, byteorder='little'))  # Chuyển length thành byte, cộng thêm 8
    buff.append(NEW_FW.to_bytes(1, byteorder='little'))  # Chuyển CMD thành byte (nếu CMD là số nguyên)
    buff.append(MAJOR.to_bytes(1, byteorder='little')) 
    buff.append(MINOR.to_bytes(1, byteorder='little')) 
    buff.append(BUILD.to_bytes(1, byteorder='little')) 
    buff.append(length.to_bytes(2, byteorder='big')) 
    # buff.append(MAJOR)  # buff đã là một đối tượng byte, không cần chuyển đổi
    buff = bytearray().join(buff)  # Kết hợp tất cả các phần tử thành một bytearray

    # Tạo bytearray từ tất cả các phần tử trong result
    result = bytearray().join(result)  # Kết hợp tất cả các phần tử thành một bytearray

    # Tính CRC16 và thêm vào cuối mảng byte
    print(len(result))
    crc = crc16(buff, 0, len(buff))  # Tính CRC16
    result = result + buff + crc.to_bytes(2, byteorder='big')  # Thêm CRC16 vào cuối
    return result  
 
def pack_data(buff, length, CMD):
    # Tạo danh sách để chứa các phần tử byte
    result = []
    buffTemp = []
    # Chuyển PID, MID, CMD thành byte và thêm vào result
    result.append(START.to_bytes(1, byteorder='little'))  # Chuyển PID thành byte (nếu PID là số nguyên)
    buffTemp.append((length+1).to_bytes(1, byteorder='little'))  # Chuyển length thành byte, cộng thêm 8
    buffTemp.append(CMD.to_bytes(1, byteorder='little'))  # Chuyển CMD thành byte (nếu CMD là số nguyên)
    buffTemp.append(buff)  # buff đã là một đối tượng byte, không cần chuyển đổi
    buffTemp = bytearray().join(buffTemp)  # Kết hợp tất cả các phần tử thành một bytearray

    # Tạo bytearray từ tất cả các phần tử trong result
    result = bytearray().join(result)  # Kết hợp tất cả các phần tử thành một bytearray

    # Tính CRC16 và thêm vào cuối mảng byte
    print(len(result))
    crc = crc16(buffTemp, 0, len(buffTemp))  # Tính CRC16
    result = result + buffTemp + crc.to_bytes(2, byteorder='big')  # Thêm CRC16 vào cuối
    return result

ser = serial.Serial(
    port=COMPORT,  # Đổi thành cổng serial của bạn (ví dụ: 'COM1' trên Windows hoặc '/dev/ttyUSB0' trên Linux)
    baudrate=115200,  # Tốc độ truyền dữ liệu (baud rate)
    timeout=1  # Thời gian chờ nếu không có dữ liệu đến
)

# Kiểm tra nếu cổng đã mở thành công
if ser.is_open:
    print("Cổng serial đã mở thành công!")
else:
    print("Không thể mở cổng serial!")


# while(True):
    # ser.write(b'Hello, UART!\n')
    # data = ser.read(100)  # Đọc tối đa 100 byte
    # print(f"Dữ liệu nhận được: {data}")

offet = 0x000000


flag_done = 0
# hashbyte = bytearray()

#wait reset signal
print("wait signal from stm32")
while True:
        if ser.in_waiting >= 1:  # Kiểm tra nếu có dữ liệu đến
            response = ser.read(1)  # Đọc dữ liệu từ UART
            if response[0] == 0xAC:
                print(f"Nhận được phản hồi: 0x{response.hex()}")
                break  # Thoát sau khi nhận được phản hồi
            print(f"Nhận được phản hồi: 0x{response.hex()}")

#send data about firmware 
with open(fwFile, "rb") as f:
    f.seek(0, 2)  # Di chuyển con trỏ đến cuối file
    file_size = f.tell()  # Lấy vị trí con trỏ, chính là kích thước file
print(f"Kích thước file: {file_size} bytes")
buff = pack_header(file_size)
print(f"Sent: {buff.hex()}")
ser.write(buff)
#wait response
print("wait signal from stm32")
while True:
        if ser.in_waiting >= 1:  # Kiểm tra nếu có dữ liệu đến
            response = ser.read(1)  # Đọc dữ liệu từ UART
            if response[0] == 0x55:
                print(f"Nhận được phản hồi: 0x{response.hex()}")
                break  # Thoát sau khi nhận được phản hồi
            print(f"Nhận được phản hồi: {response.hex()}")
  
#start transfer firmware
for chunk in read_binary_in_chunks(fwFile):
    # print(chunk)  # In từng khối byte (ở dạng bytes)
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
        if ser.in_waiting >= 1:  # Kiểm tra nếu có dữ liệu đến
            response = ser.read(1)  # Đọc dữ liệu từ UART
            # print(f"Nhận được phản hồi: {response.decode('utf-8')}")
            break  # Thoát sau khi nhận được phản hồi
    # # Đợi phản hồi (ở đây đọc tối đa 4 byte)
    # response = ser.read(2)  # Đọc 2 byte, có thể thay đổi tùy vào dữ liệu bạn mong đợi

    # Kiểm tra xem phản hồi có giống với dữ liệu kỳ vọng không
    if response == expected_response:
        print(f"Received expected response: {response.hex()}")
        offet += length
    else:
        print(f"Unexpected response: {response.hex()}")

    # break

# # Đợi phản hồi (ở đây đọc tối đa 4 byte)
# response = ser.read(1)  # Đọc 4 byte, có thể thay đổi tùy vào dữ liệu bạn mong đợi

# # Kiểm tra xem phản hồi có giống với dữ liệu kỳ vọng không
# if response == expected_response:
#     print(f"Received expected response: {response.hex()}")
# else:
#     print(f"Unexpected response: {response.hex()}")
print("transfer successfull"); 
ser.close()