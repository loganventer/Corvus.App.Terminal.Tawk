package main

// Config is the start-up payload sent by the C side ("init" command).
type Config struct {
	AuthDir  string `json:"auth_dir"`
	MediaDir string `json:"media_dir"`
	LogDir   string `json:"log_dir"`
	Debug    bool   `json:"debug"`
}
