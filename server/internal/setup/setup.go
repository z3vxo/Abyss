package setup

import (
	"crypto/rand"
	"encoding/hex"
	"errors"
	"fmt"
	"os"
	"path/filepath"

	"github.com/z3vxo/Abyss/internal/db"
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

func HomeDir() (string, error) {
	return os.UserHomeDir()
}

func CreateDirsAndFiles() (string, string, error) {
	home, err := HomeDir()
	if err != nil {
		return "", "", fmt.Errorf("resolve home dir: %w", err)
	}

	base := filepath.Join(home, ".local", "share", "abyss")

	dirs := []string{
		"database",
		"config",
		"logs",
	}

	files := []string{
		"database/abyss.db",
		"logs/access.log",
		"logs/error.log",
		"logs/general.log",
	}

	for _, d := range dirs {
		full := filepath.Join(base, d)
		if err := os.MkdirAll(full, 0700); err != nil {
			return "", "", fmt.Errorf("create dir %s: %w", full, err)
		}
	}

	for _, f := range files {
		full := filepath.Join(base, f)
		if err := os.MkdirAll(filepath.Dir(full), 0700); err != nil {
			return "", "", fmt.Errorf("create parent dir %s: %w", filepath.Dir(full), err)
		}

		file, err := os.OpenFile(full, os.O_CREATE|os.O_WRONLY, 0600)
		if err != nil {
			return "", "", fmt.Errorf("create file %s: %w", full, err)
		}
		if err := file.Close(); err != nil {
			return "", "", fmt.Errorf("close file %s: %w", full, err)
		}
	}

	fmt.Printf("[*] Created Dirs: %s\n", base)

	dbPath := filepath.Join(base, "database", "abyss.db")
	configPath := filepath.Join(base, "config", "teamserver.yaml")

	return dbPath, configPath, nil
}

func randomSecret() (string, error) {
	b := make([]byte, 32)
	if _, err := rand.Read(b); err != nil {
		return "", fmt.Errorf("generate secret: %w", err)
	}
	return hex.EncodeToString(b), nil
}

func WriteBaseConfig(path string) error {
	if info, err := os.Stat(path); err == nil && info.Size() > 0 {
		fmt.Printf("[*] Config already exists, skipping: %s\n", path)
		return nil
	} else if err != nil && !errors.Is(err, os.ErrNotExist) {
		return fmt.Errorf("stat config %s: %w", path, err)
	}

	accessSecret, err := randomSecret()
	if err != nil {
		return err
	}

	refreshSecret, err := randomSecret()
	if err != nil {
		return err
	}

	cfg := Config{
		TeamServer: TeamServerConfig{
			BindAddr:   "0.0.0.0",
			Port:       8080,
			TlsEnabled: true,
			TlsCert:    "",
			TlsKey:     "",
		},
		Auth: AuthConfig{
			Users: []User{
				{Name: "abyss", Password: "abyss-operator"},
			},
			JwtAccessSecret:  accessSecret,
			JwtRefreshSecret: refreshSecret,
			JwtAccessTTL:     "15m",
			JwtRefreshTTL:    "168h",
		},
	}

	data, err := yaml.Marshal(&cfg)
	if err != nil {
		return fmt.Errorf("marshal config: %w", err)
	}

	if err := os.MkdirAll(filepath.Dir(path), 0700); err != nil {
		return fmt.Errorf("create config dir: %w", err)
	}

	if err := os.WriteFile(path, data, 0600); err != nil {
		return fmt.Errorf("write config %s: %w", path, err)
	}

	fmt.Printf("[*] Wrote base config: %s\n", path)

	return nil
}

func Run() error {
	fmt.Println("[*] Setting up teamserver...")

	dbPath, configPath, err := CreateDirsAndFiles()
	if err != nil {
		return err
	}

	if err := db.SetupDB(dbPath); err != nil {
		return err
	}

	if err := WriteBaseConfig(configPath); err != nil {
		return err
	}

	return nil
}
