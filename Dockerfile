FROM debian:latest

# Install the necessary packages (cpprestsdk) and clean up to reduce the image size
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    libcpprest-dev \
    libssl-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Copy the source code to the container
COPY . /app

# Set the working directory
WORKDIR /app

# Compile the source code
RUN cmake -Bbuild -S. && cmake --build build

# Set the entry point
ENTRYPOINT ["./build/RestAPI"]