#!/usr/bin/bash

# postgresql database connection info
export PGHOST=127.0.0.1
export PGPORT=5432
export PGDATABASE=r00tbugbot
export PGUSER=r00tbugbot
export PGPASSWORD=r00tbugbot

export TGBOT_TOKEN="<your telegram bot token here>"

./r00tbugbot
