package config

import (
	"fmt"
	"os"

	"gopkg.in/yaml.v3"
)

type Config struct {
	TeamServer TeamServerConfig `yaml:"team_server"`
	Auth       AuthConfig       `yaml:"auth"`
}

type TeamServerConfig struct {
	BindAddr   string `yaml:"bind_addr"`
	Port       int    `yaml:"port"`
	TlsEnabled bool   `yaml:"tls_enabled"`
	TlsCert    string `yaml:"tls_cert"`
	TlsKey     string `yaml:"tls_key"`
}

type AuthConfig struct {
	Users            []User `yaml:"users"`
	JwtAccessSecret  string `yaml:"jwt_access_secret"`
	JwtRefreshSecret string `yaml:"jwt_refresh_secret"`
	JwtAccessTTL     string `yaml:"jwt_access_ttl"`
	JwtRefreshTTL    string `yaml:"jwt_refresh_ttl"`
}

type User struct {
	Name     string `yaml:"name"`
	Password string `yaml:"password"`
}

func Load(path string) (*Config, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("read config %s: %w", path, err)
	}

	var cfg Config
	if err := yaml.Unmarshal(data, &cfg); err != nil {
		return nil, fmt.Errorf("parse config %s: %w", path, err)
	}

	return &cfg, nil
}
