from __future__ import annotations
import json, time
import requests
from PyQt6.QtCore import QThread, pyqtSignal


class MagnetometerWorker(QThread):
    reading = pyqtSignal(float, float, float)   # x, y, z en uT
    rate    = pyqtSignal(float)                 # suavizado Hz
    status  = pyqtSignal(bool, str)             # (ok, message)

    def __init__(self, url: str, timeout_connect=5.0, timeout_read=60.0):
        super().__init__()
        self.url = url
        self.timeout_connect = timeout_connect
        self.timeout_read = timeout_read
        self._running = False

    def run(self):
        self._running = True
        ema = None
        t0 = time.time()
        cnt = 0

        while self._running:
            # Búfer de Python para ensamblar los datos recibidos
            buffer = b'' 
            
            try:
                with requests.Session() as s:
                    self.status.emit(False, "connecting…")
                    with s.get(self.url, stream=True,
                                 timeout=(self.timeout_connect, self.timeout_read)) as r:
                        r.raise_for_status()
                        self.status.emit(True, "connected")
                        t0 = time.time(); cnt = 0; ema = None
                        
                        print("--- CONECTADO. Esperando 'chunks' de datos del ESP32... ---")

                        # **LA SOLUCIÓN: Usar iter_content()**
                        # Lee los "trozos" de datos crudos (bytes) a medida que llegan.
                        for chunk in r.iter_content(chunk_size=128): 
                            
                            if not self._running:
                                break
                            
                            # Acumula los trozos en el búfer
                            buffer += chunk
                            
                            # Procesa todas las líneas completas (separadas por \n) que tengamos
                            while b'\n' in buffer:
                                # Separa la primera línea completa del resto del búfer
                                line_bytes, buffer = buffer.split(b'\n', 1)
                                
                                # Decodifica y limpia la línea
                                raw_clean = line_bytes.decode('utf-8').strip()
                                
                                if not raw_clean:
                                    continue

                                # --- LÓGICA DE PROCESAMIENTO ---
                                try:
                                    # Carga el JSON limpio
                                    data = json.loads(raw_clean)
                                    
                                    x = float(data.get("x", 0.0))
                                    y = float(data.get("y", 0.0))
                                    z = float(data.get("z", 0.0))
                                    self.reading.emit(x, y, z)

                                    # ... (Cálculo de Tasa Hz) ...
                                    cnt += 1
                                    now = time.time()
                                    if now - t0 >= 1.0:
                                        inst = cnt / (now - t0)
                                        ema = inst if ema is None else (0.85 * ema + 0.15 * inst)
                                        self.rate.emit(ema)
                                        t0 = now
                                        cnt = 0
                                
                                except json.JSONDecodeError:
                                    # Opcional: imprimir la línea corrupta para depurar
                                    # print(f"Línea corrupta (ignorada): {raw_clean}")
                                    continue 
                                except Exception:
                                    continue

            except Exception as e:
                error_message = str(e)
                if "bytes" in error_message and "str" in error_message:
                    error_message = "Error de decodificación en stream. Reconectando..."
                
                print(f"ERROR: {error_message}") # Imprime el error
                self.status.emit(False, f"reconnecting… ({error_message})")
                
                for _ in range(5):
                    if not self._running: break
                    time.sleep(0.5)

    def stop(self):
        self._running = False
        self.wait(2000)