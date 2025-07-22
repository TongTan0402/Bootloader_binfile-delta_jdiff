import serial
import serial.tools.list_ports
import subprocess
import time

UPDATE_FIRMWARE = 0
UPDATE_PATCH    = 1

# FSM_START_TYPE = 0 to Update Firmware
# FSM_START_TYPE = 1 to Update Patch
FSM_START_TYPE = UPDATE_FIRMWARE

selected_port = "COM3"
# path = "D:\\Documents\\Programs\\STM32\\SPL\\Blink_Led\\Objects\\Blink_Led.bin"
# path = "D:\Documents\Programs\STM32\SPL\STM32\GPIO\Objects\gpio.bin"
firmware_path = "D:\\Documents\\Programs\\STM32\\SPL\\Booloader\\Bootloader_binfile\\Resources\\file\\new.bin"
patch_path = "D:\\Documents\\Programs\\STM32\\SPL\\Booloader\\Bootloader_binfile\\Resources\\file\\patch.patch"

result = subprocess.run(["make_patch.bat"], cwd = "D:\\Documents\\Programs\\STM32\\SPL\\Booloader\\Bootloader_binfile\\Resources\\file", shell=True, capture_output=True, text=True)
print(result.stdout)

FSM_TYPE_MESSAGE_START  = 0x00
FSM_TYPE_MESSAGE_DATA   = 0x01
FSM_TYPE_MESSAGE_END    = 0x02

NUM_OF_BYTES_IN_A_FRAME = 0x200
FSM_ACK_SENT_SUCCESSFULLY = 0xff
FSM_ACK_UPDATE_FIRMWARE_SUCESSFULLY = 0x55


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
  frm += bytes([type_message])                            # Type Message
  frm += len(data).to_bytes(2, 'little')                  # Length
  frm += data                                             # Data
  frm += CheckSum(frm, len(frm)).to_bytes(2, 'little')    # Check Sum
  
  return frm


def SendMessage(frame, offset):
  ack = 0
  timeout = 10
  while(not (ack == FSM_ACK_SENT_SUCCESSFULLY or ack == FSM_ACK_UPDATE_FIRMWARE_SUCESSFULLY) and timeout):
    ser.write(frame)
    ack = int(ser.readline().strip())
    print(f"offset {format(offset, '02X')}: {frame} - Ack: {ack}")
    # PrintHex(ack.decode().split(' '))
    timeout -= 1
      
  if(not timeout): 
    print("\nError!!!\n")
    while(1):
      pass
    
  return 1


def SendFrame(path):
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
    file.close()


def UpdateFirmwareMessage():
  # Bản tin cập nhật firmware
  update_message = "AA550A24557064617465204669726D77617265212121D8\n"
  count = 2 
  while(count):
    ser.write(update_message.encode('utf-8'))
    lines = ser.readlines() # Đọc tất cả các dòng trả về trong buffer
    for line in lines:
      # Kiểm tra xem có dòng nào chứa "Preparing firmware update"
      # Nếu có, gửi "Done!!!" và chờ nhận "Start update firmware!!!"
      if("Preparing firmware update" in line.strip().decode()):
        data = ""
        while(data != "Start update firmware!!!"):
          ser.write("Done!!!\n".encode('utf-8'))
          data = ser.readline().strip().decode()
          print(data)
        break
    count -= 1
  

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
  ser = serial.Serial(port=selected_port, baudrate=115200, timeout=0.5)

  if ser.is_open:
    print("Cổng COM đã mở thành công!")
  else:
    print("Cổng COM chưa mở!")

  UpdateFirmwareMessage()
  
  # Start_Frame
  with open(firmware_path, 'rb') as file:
    length = len(file.read())
    frame = CreateFrame(bytes([FSM_START_TYPE]) + length.to_bytes(4, 'little'), FSM_TYPE_MESSAGE_START)
    file.close()
  
  # Data_Frame
  start_time = time.time()
  if(SendMessage(frame, 0)):
    if(FSM_START_TYPE == UPDATE_FIRMWARE):
      SendFrame(firmware_path)
    elif(FSM_START_TYPE == UPDATE_PATCH):
      SendFrame(patch_path)

  frame = CreateFrame(b'', FSM_TYPE_MESSAGE_END)
  SendMessage(frame, 0)
  
  total_time = time.time() - start_time
  
  print(f"\nTime: {total_time:.4f} sec")
  
  ser.close()
