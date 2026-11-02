package teamserver

import (
	"crypto/ecdsa"
	"crypto/tls"
	"crypto/x509"
	"encoding/pem"
	"os"
	"path/filepath"
	"testing"

	"github.com/z3vxo/Abyss/server/internal/config"
)

func TestTeamServerCertificate(t *testing.T) {
	cert, err := teamServerCertificate(config.TeamServerConfig{BindAddr: "127.0.0.1"})
	if err != nil {
		t.Fatal(err)
	}
	leaf, err := x509.ParseCertificate(cert.Certificate[0])
	if err != nil {
		t.Fatal(err)
	}
	if err := leaf.CheckSignatureFrom(leaf); err == nil {
		t.Fatal("server certificate should not be a CA")
	}
	if err := leaf.CheckSignature(leaf.SignatureAlgorithm, leaf.RawTBSCertificate, leaf.Signature); err != nil {
		t.Fatal(err)
	}
	if err := leaf.VerifyHostname("127.0.0.1"); err != nil {
		t.Fatal(err)
	}
	key, err := x509.MarshalECPrivateKey(cert.PrivateKey.(*ecdsa.PrivateKey))
	if err != nil {
		t.Fatal(err)
	}
	dir := t.TempDir()
	certPath := filepath.Join(dir, "cert.pem")
	keyPath := filepath.Join(dir, "key.pem")
	if err := os.WriteFile(certPath, pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: cert.Certificate[0]}), 0600); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(keyPath, pem.EncodeToMemory(&pem.Block{Type: "EC PRIVATE KEY", Bytes: key}), 0600); err != nil {
		t.Fatal(err)
	}
	loaded, err := teamServerCertificate(config.TeamServerConfig{TlsCert: certPath, TlsKey: keyPath})
	if err != nil {
		t.Fatal(err)
	}
	if string(loaded.Certificate[0]) != string(cert.Certificate[0]) {
		t.Fatal("incorrect certificate")
	}
	for _, cfg := range []config.TeamServerConfig{{TlsCert: certPath}, {TlsKey: keyPath}, {TlsCert: certPath, TlsKey: filepath.Join(dir, "missing")}} {
		if _, err := teamServerCertificate(cfg); err == nil {
			t.Fatal("expected invalid configuration to fail")
		}
	}
	_, err = tls.X509KeyPair(pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: cert.Certificate[0]}), pem.EncodeToMemory(&pem.Block{Type: "EC PRIVATE KEY", Bytes: key}))
	if err != nil {
		t.Fatal(err)
	}
}
