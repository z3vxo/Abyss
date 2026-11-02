package teamserver

import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/tls"
	"crypto/x509"
	"crypto/x509/pkix"
	"fmt"
	"math/big"
	"net"
	"time"

	"github.com/z3vxo/Abyss/server/internal/config"
)

func teamServerCertificate(cfg config.TeamServerConfig) (tls.Certificate, error) {
	if cfg.TlsCert != "" || cfg.TlsKey != "" {
		if cfg.TlsCert == "" || cfg.TlsKey == "" {
			return tls.Certificate{}, fmt.Errorf("tls_cert and tls_key must both be provided")
		}
		cert, err := tls.LoadX509KeyPair(cfg.TlsCert, cfg.TlsKey)
		if err != nil {
			return tls.Certificate{}, fmt.Errorf("load configured certificate and key: %w", err)
		}
		return cert, nil
	}

	key, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		return tls.Certificate{}, fmt.Errorf("generate TLS key: %w", err)
	}
	serial, err := rand.Int(rand.Reader, new(big.Int).Lsh(big.NewInt(1), 128))
	if err != nil {
		return tls.Certificate{}, fmt.Errorf("generate certificate serial: %w", err)
	}
	now := time.Now()
	template := &x509.Certificate{
		SerialNumber: serial,
		Subject:      pkix.Name{CommonName: "Abyss teamserver"},
		NotBefore:    now.Add(-5 * time.Minute), NotAfter: now.AddDate(1, 0, 0),
		KeyUsage:              x509.KeyUsageDigitalSignature,
		ExtKeyUsage:           []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth},
		BasicConstraintsValid: true,
		DNSNames:              []string{"localhost"},
		IPAddresses:           []net.IP{net.ParseIP("127.0.0.1"), net.ParseIP("::1")},
	}
	if ip := net.ParseIP(cfg.BindAddr); ip != nil && !ip.IsUnspecified() {
		template.IPAddresses = append(template.IPAddresses, ip)
	} else if cfg.BindAddr != "" && cfg.BindAddr != "0.0.0.0" && cfg.BindAddr != "::" && cfg.BindAddr != "localhost" {
		template.DNSNames = append(template.DNSNames, cfg.BindAddr)
	}
	der, err := x509.CreateCertificate(rand.Reader, template, template, &key.PublicKey, key)
	if err != nil {
		return tls.Certificate{}, fmt.Errorf("generate self-signed certificate: %w", err)
	}
	return tls.Certificate{Certificate: [][]byte{der}, PrivateKey: key}, nil
}
