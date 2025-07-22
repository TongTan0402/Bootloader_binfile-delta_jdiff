import serial
import serial.tools.list_ports
import time

# FSM_START_TYPE = 0 to Update Firmware
# FSM_START_TYPE = 1 to Update Patch
FSM_START_TYPE = 1

selected_port = "COM3"
# path = "D:\\Documents\\Programs\\STM32\\SPL\\Blink_Led\\Objects\\Blink_Led.bin"
# path = "D:\Documents\Programs\STM32\SPL\STM32\GPIO\Objects\gpio.bin"
# path = "D:\\Documents\\Programs\\STM32\\SPL\\Booloader\\Bootloader_binfile\\Resources\\file\\old.bin"
path = "D:\\Documents\\Programs\\STM32\\SPL\\Booloader\\Bootloader_binfile\\Resources\\file\\patch.patch"

FSM_TYPE_MESSAGE_START  = 0x00
FSM_TYPE_MESSAGE_DATA   = 0x01
FSM_TYPE_MESSAGE_END    = 0x02

NUM_OF_BYTES_IN_A_FRAME = 0x400
FSM_ACK_SENT_SUCCESSFULLY = 4


def CheckSum(data, length):
  sum = 0
  for i in range(0, length):
    sum += data[i]
  return (~sum + 1) & 0xffff

# def PrintHex(s):
#   temp = ""
#   for num in s:
#     st = str(num).strip()
#     if(st): 
#       temp += format(int(st), '02X') + " "
#   if(temp): 
#     print(f"{temp}")
    
    
def CreateFrame(data, type_message):
  frm =  bytes([0xaa, 0x55])
  frm += bytes([type_message])               # Type Message
  frm += bytes([len(data) & 0xff, len(data) >> 8])                  # Length
  frm += data                                # Data
  check_sum = CheckSum(frm, len(frm))
  print(format(check_sum, '04X'))
  frm += bytes([check_sum & 0xff, check_sum >> 8])    # Check Sum
  
  return frm


def SendMessage(frame, offset):
  ack = 0
  timeout = 10
  while(ack != FSM_ACK_SENT_SUCCESSFULLY and timeout):
    ser.write(frame)
    ack = int(ser.readline().strip())
    print(f"offset {format(offset, '02X')}: {frame} - Ack: {ack}")
    # PrintHex(ack.decode().split(' '))
    timeout -= 1
      
  if(not timeout): 
    print("\nError!!!\n")
    
  return timeout and 1


#============================================================= Main =============================================================
#============================================================= Main =============================================================
#============================================================= Main =============================================================

# Liệt kê tất cả các cổng COM
ports = list(serial.tools.list_ports.comports())

if not ports:
  print("Không tìm thấy cổng serial nào!")
else:
  index = -1
  print("Các cổng tìm thấy:")
  for i, port in enumerate(ports):
    print(f"{i}: {port.device} - {port.description}")
    if(selected_port == port.device): 
      index = i

  if(index == -1):
    idx = int(input("Chọn số cổng muốn dùng: "))
    selected_port = ports[idx].device

  print(f"Đang mở cổng: {selected_port}")

  # Mở cổng UART
  ser = serial.Serial(port=selected_port, baudrate=115200, timeout=None)

  if ser.is_open:
    print("Cổng COM đã mở thành công!")
  else:
    print("Cổng COM chưa mở!")

  # Đọc file và gửi
  
  frame = CreateFrame(bytes([FSM_START_TYPE]), FSM_TYPE_MESSAGE_START)
  
  if(SendMessage(frame, 0)):
    offset = 0
    with open(path, 'rb') as file:
      while(1):
        data = file.read(NUM_OF_BYTES_IN_A_FRAME)
        if(not data):
          break
        frame = CreateFrame(data, FSM_TYPE_MESSAGE_DATA)
        if(not SendMessage(frame, offset)):
          break
        offset += NUM_OF_BYTES_IN_A_FRAME

  frame = CreateFrame(b'', FSM_TYPE_MESSAGE_END)
  SendMessage(frame, offset)
  
  ser.close()
