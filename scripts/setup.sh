#!/bin/bash

# Set soft & hard limit for file descriptors
ulimit -n 100000

# Make it permanent by editing limits.conf
echo "* soft nofile 100000" | sudo tee -a /etc/security/limits.conf
echo "* hard nofile 100000" | sudo tee -a /etc/security/limits.conf

# Apply changes
sudo sysctl -w fs.file-max=100000
