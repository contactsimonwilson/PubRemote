Import("env")
import os
import time
import zipfile
import shutil
import re

def zip_build_files(source, target, env):
    # Define relevant paths
    project_dir = os.getcwd()
    build_dir = os.path.join(project_dir, f".pio{os.sep}build")

    # Define list of files to zip
    files_to_zip = [
        "bootloader.bin",
        "partitions.bin",
        "firmware.bin"
    ]

    # Define allowed build flags (whitelist approach for security)
    ALLOWED_FLAGS = {
        'RELEASE_VARIANT': 'dev',
        'VERSION_MAJOR': '0',
        'VERSION_MINOR': '0', 
        'VERSION_PATCH': '0'
    }

    # Get build flags for parsing
    build_flags = env.get('BUILD_FLAGS', [])

    # Retrieve build information using secure whitelist approach
    extracted_values = {}
    
    for flag in build_flags:
        # Only process flags that start with our allowed prefixes
        for allowed_flag in ALLOWED_FLAGS.keys():
            flag_prefix = f"{allowed_flag}="
            if flag.startswith(f"-D{flag_prefix}") or flag.startswith(f"-D {flag_prefix}") or flag.startswith(flag_prefix):
                # Extract the value after the equals sign
                if "=" in flag:
                    value = flag.split("=", 1)[1].strip('\\"\'')
                    # Sanitize the value to prevent path injection or other issues
                    value = re.sub(r'[^\w\-\.]', '', value)
                    extracted_values[allowed_flag] = value
                    break

    # Use extracted values or defaults
    release_variant = extracted_values.get('RELEASE_VARIANT', ALLOWED_FLAGS['RELEASE_VARIANT'])
    version_major = extracted_values.get('VERSION_MAJOR', ALLOWED_FLAGS['VERSION_MAJOR'])
    version_minor = extracted_values.get('VERSION_MINOR', ALLOWED_FLAGS['VERSION_MINOR'])
    version_patch = extracted_values.get('VERSION_PATCH', ALLOWED_FLAGS['VERSION_PATCH'])

    # Validate extracted values to ensure they're safe for filenames
    def validate_filename_component(value, component_name):
        if not re.match(r'^[\w\-\.]+$', str(value)):
            print(f"Warning: Invalid characters in {component_name}, using default")
            return ALLOWED_FLAGS.get(component_name, '0')
        return str(value)

    release_variant = validate_filename_component(release_variant, 'RELEASE_VARIANT')
    version_major = validate_filename_component(version_major, 'VERSION_MAJOR')
    version_minor = validate_filename_component(version_minor, 'VERSION_MINOR')
    version_patch = validate_filename_component(version_patch, 'VERSION_PATCH')

    version_string = f"v{version_major}.{version_minor}.{version_patch}.{release_variant}"
    
    # Build debugging - only show safe, filtered values
    print(f"Creating ZIP for release of {env['PIOENV']}: {env['PIOENV']}-{version_string}.zip")
    print(f"Extracted build info: MAJOR={version_major}, MINOR={version_minor}, PATCH={version_patch}, VARIANT={release_variant}")
    
    # Create zip in the root of the directory
    zip_filename = f"{env['PIOENV']}-{version_string}.zip"
    ZipFile = zipfile.ZipFile(os.path.join(project_dir, zip_filename), "w")
        
    # Zip relevant files
    for file in files_to_zip: 
        file_path = os.path.join(build_dir, env['PIOENV'], file)
        if os.path.exists(file_path):
            ZipFile.write(file_path, arcname=os.path.basename(file), compress_type=zipfile.ZIP_DEFLATED)
        else:
            print(f"Warning: {file} not found, skipping")

    # Close stream
    ZipFile.close()
    
    # Copy firmware.bin to versioned filename
    firmware_source = os.path.join(build_dir, env['PIOENV'], "firmware.bin")
    firmware_dest = os.path.join(project_dir, f"{env['PIOENV']}-{version_string}.bin")
    
    if os.path.exists(firmware_source):
        print(f"Copying firmware.bin to: {env['PIOENV']}-{version_string}.bin")
        shutil.copy(firmware_source, firmware_dest)
    else:
        print("Warning: firmware.bin not found, skipping copy")

# Add postbuild call
env.AddPostAction("$BUILD_DIR/firmware.bin", zip_build_files)