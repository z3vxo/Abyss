package main

import (
	"log"
	"os"

	"github.com/z3vxo/Abyss/internal/config"
	"github.com/z3vxo/Abyss/internal/setup"
	"github.com/z3vxo/Abyss/internal/teamserver"
)

func main() {
	if len(os.Args) > 1 && os.Args[1] == "setup" {
		if err := setup.Run(); err != nil {
			log.Fatal(err)
		}
	}

	if len(os.Args) > 2 && os.Args[1] == "--profile" {
		cfg, err := config.Load(os.Args[2])
		if err != nil {
			log.Fatal(err)
		}

		ts, err := teamserver.New(cfg)
		if err != nil {
			log.Fatal(err)
		}

		if err := ts.Run(); err != nil {
			log.Fatal(err)
		}
	}
}
