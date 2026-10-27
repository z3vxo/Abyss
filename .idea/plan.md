

# Server
1. startups
2. checks if arg[1] == setup
  1. if so, jump to setup.Run()
  2. this creates ~/.local/share/abyss with dirs
        /db -> abyss.db, core DB, tables
                - agents
                - files
                - listeners
                - tasks
        /logs -> auth.log, error.log, genernal.log
        /profiles -> 
                    intial example.conf, 
                    TOML based profile that defines operator(s) user:pass, port to listen operator on, http headers(for agent <-> server), whether to use tls, if so what certs etc
  3. writes intial base config to disk

3. else checks if --profile <profile> is set else errors
4. jumps to srv, err := server.Init(profilePath) -> sets up db connection, chi router, http server
5. srv.Run() -> runs server
