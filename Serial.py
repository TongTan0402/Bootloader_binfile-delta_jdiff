import serial
import serial.tools.list_ports
import subprocess
import time
import json

UPDATE_FIRMWARE = 0x00
UPDATE_PATCH    = 0x01
UPDATE_APP      = 0x02

# + Nếu update APP thì sẽ update toàn bộ firmware trực tiếp vào phân vùng application
# + Nếu update PATCH thì sẽ update bản vá lỗi vào phân vùng patch, 
#   sau đó kết hợp bản application cũ trên flash và bản patch mới để tạo ra bản firmware mới.
#   Nếu dùng FSM_START_TYPE = UPDATE_PATCH, nếu trong quá trình update patch trước đó gặp lỗi
#   thì sẽ tự động chuyển sang UPDATE_FIRMWARE.
# + Nếu update FIRMWARE thì sẽ update toàn bộ firmware vào phân vùng thứ 2, sau đó mới chuyển vào phân vùng application
FSM_START_TYPE = UPDATE_APP

selected_port = "COM4"

firmware_path = "Resources\\file\\new.bin"
patch_path    = "Resources\\file\\patch.patch"

FSM_TYPE_MESSAGE_START  = 0x00
FSM_TYPE_MESSAGE_DATA   = 0x01
FSM_TYPE_MESSAGE_END    = 0x02

NUM_OF_BYTES_IN_A_FRAME = 0x200
FSM_ACK_SENT_SUCCESSFULLY = 0xff
FSM_ACK_UPDATE_FIRMWARE_SUCESSFULLY = 0x55


json_data = {"Send_Patch": False}

def CheckSum(data, length):
  sum = 0
  for i in range(0, length):
    sum += data[i]
  return (~sum + 1) & 0xffff

    
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
    ser.flush()  # Xả buffer gửi đi ngay
    ack = int(ser.readline().strip())
    if(offset != -1):
      print(f"Addr 0x0800{format(offset, '04X')} - 0x0800{format(offset + len(frame) - 8, '04X')}: Ack: {ack}")
    timeout -= 1
      
  if(not timeout): 
    print("\nError!!!\n")
    raise ValueError("Gửi bản tin không thành công!!!")
    
  return 1


def SendFrame(path):
  offset = 0x4000
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
  timeout = 2 
  while(timeout):
    ser.write(update_message.encode('utf-8'))
    ser.flush()  # Xả buffer gửi đi ngay
    
    while(1):
      line = ser.readline()
      if(not line):
        break
      # Kiểm tra xem có dòng nào chứa "Preparing firmware update"
      # Nếu có, gửi "Done!!!" và chờ nhận "Start update firmware!!!"
      if("Preparing firmware update" in line.strip().decode()):
        data = ""
        while(data != "Start update firmware!!!"):
          ser.write("Done!!!\n".encode('utf-8'))
          ser.flush()  # Xả buffer gửi đi ngay
          data = ser.readline().strip().decode()
          print(data)
        break
    timeout -= 1
    

def Update_JsonFile(readOrWrite, data = 0):
  global json_data
  if(readOrWrite == "r"):
    with open("data.json", "r") as file:
      json_data = json.load(file)
      file.close()
  elif(readOrWrite == "w"):
    json_data["Send_Patch"] = data
    with open("data.json", "w") as file:
      json.dump(json_data, file, indent=4)
      file.close()
  

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
    raise ValueError("Không mở được cổng COM")

  # Tạo các file .bin và file .patch
  result = subprocess.run(["make_patch.bat"], cwd = "Resources\\file", shell=True, capture_output=True, text=True)
  print(result.stdout)

  # Đọc dữ liệu từ file .json
  if(FSM_START_TYPE != UPDATE_APP):
    try:
      Update_JsonFile("r")
    except:
      pass

    # Nếu Send_Patch == false thì update toàn bộ firmware
    if(json_data["Send_Patch"] == False):
      FSM_START_TYPE = UPDATE_FIRMWARE
  
  # Ghi dữ liệu vào file .json
  Update_JsonFile("w", False)

  # Gửi bản tin cập nhật firmware
  UpdateFirmwareMessage()
  
  # Start_Frame
  with open(firmware_path, 'rb') as file:
    length = len(file.read())
    frame = CreateFrame(bytes([FSM_START_TYPE]) + length.to_bytes(4, 'little'), FSM_TYPE_MESSAGE_START)
    file.close()
  
  # Data_Frame
  start_time = time.time()
  if(SendMessage(frame, -1)):
    if(FSM_START_TYPE == UPDATE_FIRMWARE or FSM_START_TYPE == UPDATE_APP):
      SendFrame(firmware_path)
    elif(FSM_START_TYPE == UPDATE_PATCH):
      SendFrame(patch_path)

  frame = CreateFrame(b'', FSM_TYPE_MESSAGE_END)
  SendMessage(frame, -1)
  
  total_time = time.time() - start_time
  
  print(f"\nTime: {total_time:.4f} sec")
  
  Update_JsonFile("w", True)
    
  ser.close()
