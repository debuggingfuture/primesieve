# Use an official C++ runtime as a base image
FROM gcc:latest

# Set the working directory in the container to /app
WORKDIR /app

# Copy the current directory contents into the container at /app
COPY . .

# Install any needed packages specified in README.md
RUN apt-get update && apt-get install -y \
    libprimesieve-dev

# Compile the project
RUN cmake .
RUN cmake --build . --parallel
RUN sudo cmake --install .
RUN sudo ldconfig

# Run the output program when the container launches
CMD ["./primesieve"]