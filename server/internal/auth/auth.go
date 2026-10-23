package auth

import (
	"context"
	"crypto/rand"
	"crypto/subtle"
	"encoding/base64"
	"encoding/json"
	"net/http"
	"strings"
	"time"

	"github.com/golang-jwt/jwt/v5"
	"github.com/z3vxo/Abyss/internal/config"
)

type Auth struct {
	JwtAccessSecret  []byte
	JwtRefreshSecret []byte
	JwtAccessTTL     time.Duration
	JwtRefreshTTL    time.Duration
	Users            map[string]Operator
}

type Operator struct {
	Password string
}

func New(cfg *config.Config) *Auth {
	users := make(map[string]Operator, len(cfg.Auth.Users))
	for _, user := range cfg.Auth.Users {
		users[user.Name] = Operator{Password: user.Password}
	}

	return &Auth{
		JwtAccessSecret:  []byte(cfg.Auth.JwtAccessSecret),
		JwtRefreshSecret: []byte(cfg.Auth.JwtRefreshSecret),
		JwtAccessTTL:     parseTTL(cfg.Auth.JwtAccessTTL, 15*time.Minute),
		JwtRefreshTTL:    parseTTL(cfg.Auth.JwtRefreshTTL, 168*time.Hour),
		Users:            users,
	}
}

func parseTTL(value string, fallback time.Duration) time.Duration {
	d, err := time.ParseDuration(value)
	if err != nil {
		return fallback
	}
	return d
}

type Tokens struct {
	AccessToken  string `json:"access"`
	RefreshToken string `json:"refesh"`
}

func (auth *Auth) Generate(user string) (Tokens, error) {
	access, err := auth.sign(user, auth.JwtAccessSecret, auth.JwtAccessTTL)
	if err != nil {
		return Tokens{}, err
	}

	refresh, err := auth.sign(user, auth.JwtRefreshSecret, auth.JwtRefreshTTL)
	if err != nil {
		return Tokens{}, err
	}
	return Tokens{AccessToken: access, RefreshToken: refresh}, nil
}

func (auth *Auth) sign(username string, sec []byte, ttl time.Duration) (string, error) {
	now := time.Now()

	id, err := genID()
	if err != nil {
		return "", err
	}

	claims := Claims{
		Username: username,
		RegisteredClaims: jwt.RegisteredClaims{
			ID:        id,
			Subject:   username,
			IssuedAt:  jwt.NewNumericDate(now),
			ExpiresAt: jwt.NewNumericDate(now.Add(ttl)),
		},
	}
	return jwt.NewWithClaims(jwt.SigningMethodHS256, claims).SignedString(sec)
}

type Claims struct {
	Username string `json:"username"`
	jwt.RegisteredClaims
}

func (auth *Auth) Verify(token string) (*Claims, error) {
	return auth.verify(token, auth.JwtAccessSecret)
}

func (auth *Auth) VerifyRefresh(token string) (*Claims, error) {
	return auth.verify(token, auth.JwtRefreshSecret)
}

func genID() (string, error) {
	b := make([]byte, 16)
	if _, err := rand.Read(b); err != nil {
		return "", err
	}
	return base64.RawURLEncoding.EncodeToString(b), nil
}

func (auth *Auth) verify(tokenStr string, secret []byte) (*Claims, error) {
	claims := &Claims{}

	token, err := jwt.ParseWithClaims(tokenStr, claims, func(token *jwt.Token) (any, error) {
		if token.Method != jwt.SigningMethodHS256 {
			return nil, jwt.ErrSignatureInvalid
		}
		return secret, nil
	}, jwt.WithValidMethods([]string{"HS256"}))
	if err != nil {
		return nil, err
	}
	if !token.Valid {
		return nil, err
	}
	return claims, nil
}

type contextKey string

const claimsKey contextKey = "claims"

func (auth *Auth) AuthWare() func(http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			header := r.Header.Get("Authorization")
			scheme, tokenStr, ok := strings.Cut(header, " ")
			if !ok || !strings.EqualFold(scheme, "Bearer") || tokenStr == "" {
				w.Header().Set("Content-Type", "application/json")
				w.WriteHeader(http.StatusUnauthorized)
				json.NewEncoder(w).Encode(map[string]string{"error": "unauthorized"})
				return
			}
			claims, err := auth.Verify(tokenStr)
			if err != nil {
				w.Header().Set("Content-Type", "application/json")
				w.WriteHeader(http.StatusUnauthorized)
				json.NewEncoder(w).Encode(map[string]string{"error": "unauthorized"})
			}
			ctx := context.WithValue(r.Context(), claimsKey, claims)
			next.ServeHTTP(w, r.WithContext(ctx))
		})

	}
}

func (auth *Auth) CheckCreds(user, pass string) bool {
	op, ok := auth.Users[user]
	if !ok {
		return false
	}
	return subtle.ConstantTimeCompare([]byte(op.Password), []byte(pass)) == 1
}
