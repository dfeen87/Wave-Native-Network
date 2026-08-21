FROM nvcr.io/nvidia/pytorch:24.01-py3
WORKDIR /app
COPY . .
RUN pip install --no-cache-dir fastapi uvicorn pycuda tensorrt
EXPOSE 8000
CMD ["uvicorn", "server:app", "--host", "0.0.0.0", "--port", "8000"]
