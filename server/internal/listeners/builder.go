package listeners

import (
	"crypto/rand"
	"crypto/rsa"
	"crypto/tls"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/base64"
	"encoding/pem"
	"fmt"
	"math/big"
	"net"
	"net/http"
	"strconv"
	"strings"
	"time"

	"github.com/go-chi/chi"
	"github.com/z3vxo/Abyss/server/internal/shared"
)

func BuildTLS(info shared.HttpListenerInfo, typ ListenerType) (*tls.Config, error) {
	if typ != TypeHTTPS {
		return nil, nil
	}

	cfg := &tls.Config{MinVersion: tls.VersionTLS12}

	switch {
	case info.TlsCert == "" && info.TlsKey == "":
		cert, err := GenerateSelfSigned(info.Hostname)
		if err != nil {
			return nil, err
		}
		cfg.Certificates = []tls.Certificate{cert}
	case info.TlsCert == "" || info.TlsKey == "":
		return nil, fmt.Errorf("tls_cert and tls_key must be provided together")
	default:
		certPEM, err := base64.StdEncoding.DecodeString(info.TlsCert)
		if err != nil {
			return nil, fmt.Errorf("decode tls_cert: %w", err)
		}
		keyPEM, err := base64.StdEncoding.DecodeString(info.TlsKey)
		if err != nil {
			return nil, fmt.Errorf("decode tls_key: %w", err)
		}
		cert, err := tls.X509KeyPair(certPEM, keyPEM)
		if err != nil {
			return nil, fmt.Errorf("load tls keypair: %w", err)
		}
		cfg.Certificates = []tls.Certificate{cert}
	}

	return cfg, nil
}

func BuildServer(info shared.HttpListenerInfo, tlsCfg *tls.Config, agent AgentHandler) (*http.Server, error) {
	if agent == nil {
		return nil, fmt.Errorf("agent handler is required")
	}
	paths, err := normalizePaths(info.HttpPaths)
	if err != nil {
		return nil, err
	}

	handler := func(w http.ResponseWriter, r *http.Request) {
		if info.Hostname != "" && !hostMatches(r.Host, info.Hostname) {
			http.NotFound(w, r)
			return
		}
		agent.HandlePost(w, r)
	}

	r := chi.NewRouter()
	for _, route := range paths {
		r.Post(route, handler)
	}

	return &http.Server{
		Addr:              net.JoinHostPort(info.Bind, strconv.Itoa(info.Port)),
		Handler:           r,
		TLSConfig:         tlsCfg,
		ReadHeaderTimeout: 10 * time.Second,
		IdleTimeout:       60 * time.Second,
		MaxHeaderBytes:    1 << 20,
	}, nil
}

func hostMatches(requestHost, expected string) bool {
	expected = hostOnly(expected)
	if expected == "" {
		return false
	}
	return strings.EqualFold(hostOnly(requestHost), expected)
}

func hostOnly(hostport string) string {
	if host, _, err := net.SplitHostPort(hostport); err == nil {
		return host
	}
	return strings.Trim(hostport, "[]")
}

func normalizePaths(paths []string) ([]string, error) {
	seen := make(map[string]struct{}, len(paths))
	out := make([]string, 0, len(paths))

	for _, p := range paths {
		if p == "" || p[0] != '/' {
			return nil, fmt.Errorf("invalid http path %q: must start with /", p)
		}
		if _, ok := seen[p]; ok {
			continue
		}
		seen[p] = struct{}{}
		out = append(out, p)
	}

	if len(out) == 0 {
		return nil, fmt.Errorf("at least one http path is required")
	}

	return out, nil
}

func GenerateSelfSigned(hostname string) (tls.Certificate, error) {
	if hostname == "" {
		hostname = "localhost"
	}

	key, err := rsa.GenerateKey(rand.Reader, 2048)
	if err != nil {
		return tls.Certificate{}, fmt.Errorf("generate key: %w", err)
	}

	serial, err := rand.Int(rand.Reader, new(big.Int).Lsh(big.NewInt(1), 128))
	if err != nil {
		return tls.Certificate{}, fmt.Errorf("generate serial: %w", err)
	}

	template := &x509.Certificate{
		SerialNumber:          serial,
		Subject:               pkix.Name{CommonName: hostname},
		NotBefore:             time.Now().Add(-5 * time.Minute),
		NotAfter:              time.Now().Add(365 * 24 * time.Hour),
		KeyUsage:              x509.KeyUsageDigitalSignature | x509.KeyUsageKeyEncipherment,
		ExtKeyUsage:           []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth},
		BasicConstraintsValid: true,
	}

	if ip := net.ParseIP(hostname); ip != nil {
		template.IPAddresses = []net.IP{ip}
	} else {
		template.DNSNames = []string{hostname}
	}

	der, err := x509.CreateCertificate(rand.Reader, template, template, &key.PublicKey, key)
	if err != nil {
		return tls.Certificate{}, fmt.Errorf("create certificate: %w", err)
	}

	keyPEM := pem.EncodeToMemory(&pem.Block{Type: "RSA PRIVATE KEY", Bytes: x509.MarshalPKCS1PrivateKey(key)})
	certPEM := pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: der})

	return tls.X509KeyPair(certPEM, keyPEM)
}
