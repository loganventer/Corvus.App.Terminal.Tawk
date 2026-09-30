package main

import (
	"bytes"
	"context"
	"errors"
	"html"
	"image"
	"image/draw"
	"image/jpeg"
	_ "image/png"
	"io"
	"net"
	"net/http"
	"net/url"
	"regexp"
	"strings"
	"syscall"
	"time"
)

// Fetching a page for a link preview happens only when the user turned
// link previews on, and only for links they send. The limits keep a slow
// or huge page from holding the send, and the dialer refuses addresses on
// this machine or its local network, so a link cannot make tawk probe them.
const (
	previewTimeout   = 5 * time.Second
	previewPageLimit = 1 << 20
	previewImgLimit  = 2 << 20
	previewThumbSide = 192
)

var (
	ogTag      = regexp.MustCompile(`(?is)<meta\s[^>]*>`)
	ogProperty = regexp.MustCompile(`(?is)(?:property|name)\s*=\s*["']([^"']+)["']`)
	ogContent  = regexp.MustCompile(`(?is)content\s*=\s*["']([^"']*)["']`)
	titleTag   = regexp.MustCompile(`(?is)<title[^>]*>(.*?)</title>`)
)

// LinkPreview is what a page says about itself.
type LinkPreview struct {
	URL, Title, Description, Image string
	Thumb                          []byte
}

// publicOnly refuses connections to loopback, private, link-local and
// other non-public addresses.
func publicOnly(_, address string, _ syscall.RawConn) error {
	host, _, err := net.SplitHostPort(address)
	if err != nil {
		return err
	}
	ip := net.ParseIP(host)
	if ip == nil || ip.IsLoopback() || ip.IsPrivate() || ip.IsLinkLocalUnicast() || ip.IsLinkLocalMulticast() ||
		ip.IsUnspecified() || ip.IsMulticast() || ip.IsInterfaceLocalMulticast() {
		return errors.New("not a public address")
	}
	return nil
}

func previewClient() *http.Client {
	dialer := &net.Dialer{Timeout: previewTimeout, Control: publicOnly}
	return &http.Client{
		Timeout:   previewTimeout,
		Transport: &http.Transport{DialContext: dialer.DialContext, TLSHandshakeTimeout: previewTimeout},
		CheckRedirect: func(req *http.Request, via []*http.Request) error {
			if len(via) >= 3 || req.URL.Scheme != "https" {
				return errors.New("redirect refused")
			}
			return nil
		},
	}
}

func fetchLimited(ctx context.Context, client *http.Client, target string, limit int64) ([]byte, string, error) {
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, target, nil)
	if err != nil {
		return nil, "", err
	}
	req.Header.Set("User-Agent", "Mozilla/5.0 (compatible; tawk link preview)")
	resp, err := client.Do(req)
	if err != nil {
		return nil, "", err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return nil, "", errors.New(resp.Status)
	}
	data, err := io.ReadAll(io.LimitReader(resp.Body, limit))
	return data, resp.Header.Get("Content-Type"), err
}

// parseOpenGraph reads og:title, og:description and og:image (with the
// page title and description meta as fallbacks) from a page.
func parseOpenGraph(page string) (title, desc, img string) {
	values := map[string]string{}
	for _, tag := range ogTag.FindAllString(page, 200) {
		p := ogProperty.FindStringSubmatch(tag)
		c := ogContent.FindStringSubmatch(tag)
		if p != nil && c != nil {
			key := strings.ToLower(p[1])
			if _, seen := values[key]; !seen {
				values[key] = html.UnescapeString(strings.TrimSpace(c[1]))
			}
		}
	}
	title = values["og:title"]
	if title == "" {
		if m := titleTag.FindStringSubmatch(page); m != nil {
			title = html.UnescapeString(strings.TrimSpace(m[1]))
		}
	}
	desc = values["og:description"]
	if desc == "" {
		desc = values["description"]
	}
	return clip(title, 200), clip(desc, 400), values["og:image"]
}

func clip(s string, n int) string {
	s = strings.Join(strings.Fields(s), " ")
	if r := []rune(s); len(r) > n {
		return string(r[:n])
	}
	return s
}

// fetchLinkPreview builds a preview for an https link, or returns an error;
// the message is then sent without one.
func fetchLinkPreview(ctx context.Context, link string) (*LinkPreview, error) {
	u, err := url.Parse(link)
	if err != nil || u.Scheme != "https" || u.Host == "" {
		return nil, errors.New("only https links get a preview")
	}
	ctx, cancel := context.WithTimeout(ctx, 2*previewTimeout)
	defer cancel()
	client := previewClient()
	page, kind, err := fetchLimited(ctx, client, link, previewPageLimit)
	if err != nil || !strings.Contains(strings.ToLower(kind), "html") {
		return nil, errors.New("no page to preview")
	}
	title, desc, img := parseOpenGraph(string(page))
	if title == "" && desc == "" {
		return nil, errors.New("the page says nothing about itself")
	}
	p := &LinkPreview{URL: link, Title: title, Description: desc}
	if img != "" {
		if ref, err := u.Parse(img); err == nil && ref.Scheme == "https" {
			if data, _, err := fetchLimited(ctx, client, ref.String(), previewImgLimit); err == nil {
				p.Thumb = previewThumb(data)
			}
		}
	}
	return p, nil
}

// previewThumb is the middle square of a JPEG or PNG, as a small JPEG.
func previewThumb(data []byte) []byte {
	src, _, err := image.Decode(bytes.NewReader(data))
	if err != nil {
		return nil
	}
	b := src.Bounds()
	side := min(b.Dx(), b.Dy())
	if side <= 0 {
		return nil
	}
	square := image.NewRGBA(image.Rect(0, 0, side, side))
	draw.Draw(square, square.Bounds(), src, image.Pt(b.Min.X+(b.Dx()-side)/2, b.Min.Y+(b.Dy()-side)/2), draw.Src)
	var buf bytes.Buffer
	if err := jpeg.Encode(&buf, scaleBox(square, previewThumbSide), &jpeg.Options{Quality: 70}); err != nil || buf.Len() > 60*1024 {
		return nil
	}
	return buf.Bytes()
}
