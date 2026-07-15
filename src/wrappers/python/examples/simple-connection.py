import logging
from itscam.client import ItscamClient

logging.basicConfig(level=logging.DEBUG)

# remember to unsubscribe other connections to this port before running this test, or it will fail with "connection failed" error
try:
    with ItscamClient("192.168.255.254", port=60000) as client:
        print("Connection successful!")
except Exception as e:
    print(f"Error: {e}")
