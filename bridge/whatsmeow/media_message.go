package main

import (
	"context"
	"errors"
	"os"

	"go.mau.fi/whatsmeow"
	"go.mau.fi/whatsmeow/proto/waE2E"
	"google.golang.org/protobuf/proto"
)

const maxMediaBytes = 100 * 1024 * 1024

var errMediaFile = errors.New("the file cannot be sent")

// buildMediaMessage uploads a file inside the media folder and wraps it as a
// photo, video, audio file or document, the same way for chats and statuses.
func (s *Session) buildMediaMessage(ctx context.Context, kind, path, mime, name, text string) (*waE2E.Message, error) {
	if !s.insideMediaDir(path) {
		return nil, errMediaFile
	}
	info, err := os.Stat(path)
	if err != nil || !info.Mode().IsRegular() || info.Size() == 0 || info.Size() > maxMediaBytes {
		return nil, errMediaFile
	}
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}
	appInfo := map[string]whatsmeow.MediaType{
		"image": whatsmeow.MediaImage, "video": whatsmeow.MediaVideo, "audio": whatsmeow.MediaAudio,
	}[kind]
	if appInfo == "" {
		appInfo = whatsmeow.MediaDocument
	}
	up, err := s.client.Upload(ctx, data, appInfo)
	if err != nil {
		return nil, err
	}
	var caption *string
	if text != "" {
		caption = proto.String(text)
	}
	msg := &waE2E.Message{}
	switch appInfo {
	case whatsmeow.MediaImage:
		msg.ImageMessage = &waE2E.ImageMessage{URL: proto.String(up.URL), DirectPath: proto.String(up.DirectPath),
			MediaKey: up.MediaKey, FileEncSHA256: up.FileEncSHA256, FileSHA256: up.FileSHA256,
			FileLength: proto.Uint64(up.FileLength), Mimetype: proto.String(mime), Caption: caption}
	case whatsmeow.MediaVideo:
		msg.VideoMessage = &waE2E.VideoMessage{URL: proto.String(up.URL), DirectPath: proto.String(up.DirectPath),
			MediaKey: up.MediaKey, FileEncSHA256: up.FileEncSHA256, FileSHA256: up.FileSHA256,
			FileLength: proto.Uint64(up.FileLength), Mimetype: proto.String(mime), Caption: caption}
	case whatsmeow.MediaAudio:
		msg.AudioMessage = &waE2E.AudioMessage{URL: proto.String(up.URL), DirectPath: proto.String(up.DirectPath),
			MediaKey: up.MediaKey, FileEncSHA256: up.FileEncSHA256, FileSHA256: up.FileSHA256,
			FileLength: proto.Uint64(up.FileLength), Mimetype: proto.String(mime)}
	default:
		if name == "" {
			name = "file"
		}
		msg.DocumentMessage = &waE2E.DocumentMessage{URL: proto.String(up.URL), DirectPath: proto.String(up.DirectPath),
			MediaKey: up.MediaKey, FileEncSHA256: up.FileEncSHA256, FileSHA256: up.FileSHA256,
			FileLength: proto.Uint64(up.FileLength), Mimetype: proto.String(mime),
			FileName: proto.String(name), Title: proto.String(name), Caption: caption}
	}
	return msg, nil
}
