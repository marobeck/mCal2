from nacl.signing import VerifyKey
from nacl.exceptions import BadSignatureError

def verify_request(headers, body):
    # TODO: Implement this function to verify incoming requests from clients
    
    # key_id = headers.get("X-Key-ID")
    # signature = headers.get("X-Signature")

    # Lookup public key from DB
    # Verify signature over body
    # Return True/False

    return True  # stub