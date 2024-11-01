import os
import time
import aiohttp
import asyncio


def get_latest_files(directory):
    files = [os.path.join(directory, f) for f in os.listdir(directory) if f.endswith(".wav")]
    if not files:
        return []
    files.sort(key=os.path.getctime, reverse=True)
    return files


async def upload_file_to_api(file_path):
    url = "http://<ip address>:<port>/upload-wav/"
    async with aiohttp.ClientSession() as session:
        with open(file_path, "rb") as f:
            form_data = aiohttp.FormData()
            form_data.add_field("file", f, filename=os.path.basename(file_path), content_type="audio/wav")

            async with session.post(url, data=form_data) as response:
                if response.status == 200:
                    print(f"File {file_path} uploaded successfully. Response: {await response.json()}")
                else:
                    print(f"Failed to upload {file_path}. Response: {await response.text()}")


async def watch_directory_and_upload(directory, interval=8):
    while True:
        latest_files = get_latest_files(directory)
        if len(latest_files) >= 2:
            second_latest_file = latest_files[1]
            await upload_file_to_api(second_latest_file)
        else:
            print("No enough files found.")
        await asyncio.sleep(interval)


if __name__ == "__main__":
    directory_to_watch = "/app/audio"
    loop = asyncio.get_event_loop()
    loop.run_until_complete(watch_directory_and_upload(directory_to_watch, interval=8))
