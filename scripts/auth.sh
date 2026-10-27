REFRESH=$(curl -s http://127.0.0.1:8080/rest/auth/login -X POST -d '{"username":"abyss", "password":"abyss-operator"}' | jq -r '.refesh')

curl -s http://127.0.0.1:8080/rest/auth/refresh \
  -H 'Content-Type: application/json' \
  -d "{\"refresh\":\"$REFRESH\"}"
