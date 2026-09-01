#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_dir"

python3 -m unittest discover -s tests -v

cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I CubeIDEProject/Core/Inc \
  tests/network_alert_logic_test.c \
  CubeIDEProject/Core/Src/network_alert_logic.c \
  -o /tmp/vital_network_alert_logic_test
/tmp/vital_network_alert_logic_test

cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I CubeIDEProject/Core/Inc \
  tests/vitals_parser_test.c \
  CubeIDEProject/Core/Src/vitals_parser.c \
  -o /tmp/vital_parser_test
/tmp/vital_parser_test
