#!/bin/bash

. ./test.env

exec ./critcache-client "$client_bind" "$server" get bbcb9199-eefc-4b2f-9f76-3b332d7a9d4f

