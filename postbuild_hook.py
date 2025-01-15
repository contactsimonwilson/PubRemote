Import("env")
import os
import zipfile

def post_program_action(source, target, env):
    # Define relevant paths
    project_dir = os.getcwd()
    build_dir = os.path.join(project_dir, ".pio" + os.sep + "build")

    # Define list of files to zip
    files_to_zip = [
        "firmware.bin",
        "partitions.bin",
        "bootloader.bin"
    ]

    # Build debugging
    print("Creating ZIP for release of " + env["PIOENV"])
    
    # Create zip in the root of the directory
    ZipFile = zipfile.ZipFile(project_dir + os.sep + env["PIOENV"] + ".zip", "w" )
        
    #  Zip relevant files
    for file in files_to_zip: 
        ZipFile.write(os.path.join(build_dir, env["PIOENV"], file), arcname=os.path.basename(file), compress_type=zipfile.ZIP_DEFLATED)

    # Close stream
    ZipFile.close()

# Build if github is in the environment
# TODO | Trigger on local environment flag as well
if "GITHUB_ACTION" in os.environ:
    # Will only run if the build was not cached
    env.AddPostAction("buildprog", post_program_action)

    # # Alternative that will always run
    # env.AddPostAction("checkprogsize", post_program_action)