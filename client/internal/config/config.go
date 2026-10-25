package config

import (
	"os"

	"gopkg.in/yaml.v3"
)

type Connection struct {
	Host     string `yaml:"host"`
	Port     int    `yaml:"port"`
	Username string `yaml:"username"`
	Password string `yaml:"password"`
}

type Logging struct {
	Level   string `yaml:"level"`
	LogFile string `yaml:"logfile"`
}

type Config struct {
	ConnectionCfg Connection `yaml:"connection"`
	LoggingCfg    Logging    `yaml:"logging"`
	Debug         bool       `yaml:"-"`
}

func New(cfgPath string, debug bool) (*Config, error) {
	data, err := os.ReadFile(cfgPath)
	if err != nil {
		return nil, err
	}
	var cfg Config
	if err := yaml.Unmarshal(data, &cfg); err != nil {
		return nil, err
	}
	cfg.Debug = debug

	return &cfg, nil
}
