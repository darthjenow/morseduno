import time
from socket import timeout
import serial
import serial.tools.list_ports
from consolemenu import *
from consolemenu.items import *
from pathlib import Path
import configparser
import os

BAUDRATE = 9600
CONFIG_PATH = Path(os.environ["APPDATA" if "APPDATA" in os.environ else "XDG_CONFIG_HOME"]) / Path("morseduno/config.ini")

config = configparser.ConfigParser()

com_ports = []

# def select_comport(comport_selection):
# 	global selected_comport
# 	selected_comport = comport_selection

# prompt a string and send it
def send_string(com_port):
	send_string = input("string to send: ")

	send_serial(send_string, com_port)

# promt a file and send the content
def send_file(com_port):
	send_file_path = Path(input("path of the file to send: "))

	if send_file_path.exists():
		message = send_file_path.read_text(encoding="utf8")

		send_serial(message, com_port)
	
def send_serial(message, com_port):
	port = com_port.selected_option

	if 0 <= port < len(com_ports):
		config["comport"]["port_name"] = com_ports[port]["port"]

	try:
		# add a line terminator to the message if there isn't already one
		if message.encode("ascii")[-1] not in (10, 13):
			message += '\n'

		# open the serial port
		ser = serial.Serial(config["comport"]["port_name"], BAUDRATE, timeout=1)

		print(f"port name: {ser.name}")

		# wait 2 seconds until the arduino booted
		time.sleep(2)

		# send the text over serial
		ser.write(message.encode("utf-8"))

		ser.close()

		print ("message sent")
	except:
		print ("you selected an invalid COM-port")

	input("press Enter to return to the Menu ")

# load the config file
def load_config():
	global config

	if not CONFIG_PATH.exists():
		CONFIG_PATH.parent.mkdir(exist_ok=True, parents=True)
		
		CONFIG_PATH.touch()

		config["comport"] = { "port_name": "" }
	else:
		config.read(CONFIG_PATH)

def main():
	load_config()

	global com_ports
	com_ports = []

	for port, desc, hwid in sorted(serial.tools.list_ports.comports()):
		print (f"port: {port}, desc: {desc}, hwid: {hwid}")
		
		com_ports.append({ "port": port, "desc": desc })

	# Create the menu
	menu = ConsoleMenu("Morseduno sender", "Programm the Morseduno with text")

	# A SelectionMenu constructs a menu from a list of strings
	port_selection_menu = SelectionMenu([f"{port['port']} - {port['desc']}" for port in com_ports])

	send_string_item = FunctionItem("Send a string", send_string, [port_selection_menu])
	send_file_item = FunctionItem("Send the content of a file", send_file, [port_selection_menu])


	port_selection_submenu = SubmenuItem("Select the COM-Port you want to use", port_selection_menu, menu)

	menu.append_item(send_string_item)
	menu.append_item(send_file_item)
	menu.append_item(port_selection_submenu)

	menu.show()


if __name__ == "__main__":
	load_config()

	main()

	with CONFIG_PATH.open("w", encoding="utf-8") as f:
		config.write(f)