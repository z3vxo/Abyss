package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
	"time"
)

const (
	defaultTargetURL = "http://127.0.0.1:8081/test"
	cmdTypeRegister  = int32(1)
)

func main() {
	targetURL := defaultTargetURL
	if v := os.Getenv("ABYSS_TEST_URL"); v != "" {
		targetURL = v
	}

	body, err := registerPayload()
	if err != nil {
		fmt.Fprintf(os.Stderr, "[!] build register payload: %v\n", err)
		os.Exit(1)
	}

	req, err := http.NewRequest(http.MethodPost, targetURL, bytes.NewReader(body))
	if err != nil {
		fmt.Fprintf(os.Stderr, "[!] build request: %v\n", err)
		os.Exit(1)
	}
	req.Header.Set("Content-Type", "application/octet-stream")

	client := &http.Client{Timeout: 10 * time.Second}
	resp, err := client.Do(req)
	if err != nil {
		fmt.Fprintf(os.Stderr, "[!] POST %s: %v\n", targetURL, err)
		os.Exit(1)
	}
	defer resp.Body.Close()

	respBody, err := io.ReadAll(io.LimitReader(resp.Body, 1<<20))
	if err != nil {
		fmt.Fprintf(os.Stderr, "[!] read response: %v\n", err)
		os.Exit(1)
	}

	fmt.Printf("[*] POST %s -> HTTP %d: %s\n", targetURL, resp.StatusCode, strings.TrimSpace(string(respBody)))
	if resp.StatusCode != http.StatusOK {
		os.Exit(1)
	}
}

func registerPayload() ([]byte, error) {
	var b bytes.Buffer

	values := []int32{
		cmdTypeRegister,
		int32(time.Now().UnixNano() & 0x7fffffff),
		1,
		9,
		1234,
		5678,
		4321,
		0,
		10,
		19045,
	}

	stringsToWrite := []string{
		"abyss",
		"DESKTOP-TEST",
		"TESTDOMAIN",
		`C:\Windows\System32\test.exe`,
	}

	if err := binary.Write(&b, binary.LittleEndian, values[0:2]); err != nil {
		return nil, err
	}
	for _, s := range stringsToWrite {
		if err := binary.Write(&b, binary.LittleEndian, int32(len(s))); err != nil {
			return nil, err
		}
		if _, err := b.WriteString(s); err != nil {
			return nil, err
		}
	}
	if err := binary.Write(&b, binary.LittleEndian, values[2:]); err != nil {
		return nil, err
	}

	return b.Bytes(), nil
}
