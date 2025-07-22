#!/bin/sh

# vars for various things
GITHUB = "https://github.com/Thoq-jar/Mocha.git"
TMP_DIR = "/tmp/mocha-wm-install"
CONFIG_DIR="$HOME/.config/mocha"

# log with mocha prefix
log () {
  echo "[ Mocha ] $1"
}

# simple util to check if all tools needed are installed
check_env () {
  log "Checking env variables..."
  [ -z "$HOME" ] && log "missing: HOME" && exit || log "found: HOME"
  command -v curl >/dev/null 2>&1 && log "found: curl" || { log "missing: curl"; exit 1; }
  command -v git >/dev/null 2>&1 && log "found: git" || { log "missing: git"; exit 1; }
  command -v make >/dev/null 2>&1 && log "found: make" || { log "missing: make"; exit 1; }
}

# checks for config directory and if it doesnt exist
# it creates the config directory
ensure_config_dir () {
  log "Creating config directory"
  mkdir -p "$CONFIG_DIR" >/dev/null 2>&1
  log "Config directory created!"
}

# install the wm
install_mocha {
  git clone $GITHUB $TMP_DIR
  cd $TMP_DIR

  make -j"$(nproc)"
  sudo mv mocha-wm /usr/local/bin/mocha
  mv config/* ~/.config/mocha

  cd $HOME
  rm -rf $TMP_DIR
}

# main function
main () {
  sudo -i # get sudo
  check_env
  ensure_config_dir
  install_config
  install_mocha

  exit 0
}

# entry
main
