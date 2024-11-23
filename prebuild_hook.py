Import("env")
from datetime import datetime
import hashlib

def generate_build_id():
    # Get current timestamp
    timestamp = datetime.now().strftime("%Y%m%d_%H")
    
    # Create a hash of timestamp for shorter unique ID
    hash_object = hashlib.md5(timestamp.encode())
    build_hash = hash_object.hexdigest()[:8]
    
    # Combine timestamp and hash
    build_id = f"{build_hash}"
    return build_id

# Add build ID to environment
build_id = generate_build_id()
env.Append(BUILD_FLAGS=[f'-D BUILD_ID=\\"{build_id}\\"'])