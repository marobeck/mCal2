import subprocess
import time
import os
import shutil

# Create temp directory structure
tmp_dir = "tmp"
os.makedirs(tmp_dir, exist_ok=True)

# Create client directories
client1_dir = os.path.join(tmp_dir, "client1")
client2_dir = os.path.join(tmp_dir, "client2")
os.makedirs(client1_dir, exist_ok=True)
os.makedirs(client2_dir, exist_ok=True)

# Copy executable and assets to client directories
shutil.copy("../../../build/client/mcal2", client1_dir)
shutil.copytree("../../../assets", os.path.join(client1_dir, "assets"), dirs_exist_ok=True)
shutil.copy("../../../build/client/mcal2", client2_dir)
shutil.copytree("../../../assets", os.path.join(client2_dir, "assets"), dirs_exist_ok=True)

# Set server database path
server_db_path = os.path.join(tmp_dir, "server.db")
os.environ["MCAL_SERVER_DB"] = os.path.abspath(server_db_path)

# Start server
server = subprocess.Popen(["python3", "-m", "uvicorn", "server.main:app", "--host", "127.0.0.1", "--port", "8001"], cwd="../../../")
time.sleep(1)

# Start clients from their temp directories
client1 = subprocess.Popen(["./mcal2"], cwd=client1_dir)
client2 = subprocess.Popen(["./mcal2"], cwd=client2_dir)

# Await exit of clients
client1.wait()
client2.wait()

# Terminate processes
server.terminate()
client1.terminate()
client2.terminate()

# Clean up temp directory
shutil.rmtree(tmp_dir)
