#!/bin/bash
ps -e | grep YChatServer | awk '{ print $1; }' | xargs kill -9
