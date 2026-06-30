import serial
import time

port = 'COM10'
baud = 115200

try:
    s = serial.Serial(port, baud, timeout=1)
    print(f"--- Conectado a {port} a {baud} baudios ---")
    
    # Leer el puerto serial por 15 segundos para capturar varios ciclos
    t0 = time.time()
    while time.time() - t0 < 15:
        line = s.readline()
        if line:
            # Imprimir decodificando de bytes a string
            print(line.decode('utf-8', errors='replace').strip())
            
    s.close()
    print("--- Fin de la lectura ---")
except Exception as e:
    print(f"Error al abrir el puerto: {e}")
