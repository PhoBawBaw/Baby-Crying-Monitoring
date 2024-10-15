import os
import time
import requests


def get_latest_file(directory):
    files = [os.path.join(directory, f) for f in os.listdir(directory) if f.endswith(".wav")]
    if not files:
        return None
    latest_file = max(files, key=os.path.getctime)
    return latest_file


def upload_file_to_api(file_path):
    url = "http://<ip address>:<port>/upload-wav/"
    with open(file_path, "rb") as f:
        files = {"file": f}
        response = requests.post(url, files=files)

    if response.status_code == 200:
        print(f"File {file_path} uploaded successfully. Response: {response.json()}")
    else:
        print(f"Failed to upload {file_path}. Response: {response.text}")


def watch_directory_and_upload(directory, interval=10):
    while True:
        latest_file = get_latest_file(directory)
        if latest_file:
            upload_file_to_api(latest_file)
        else:
            print("No new files found.")
        time.sleep(interval)


if __name__ == "__main__":
    directory_to_watch = "/app/audio"
    watch_directory_and_upload(directory_to_watch, interval=10)
