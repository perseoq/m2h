# Markdown to HTML Converter (m2h)

A CLI tool that converts Markdown files to HTML with a responsive table of contents sidebar, similar to ReadTheDocs.

## Features

- Converts Markdown to clean HTML
- Generates a responsive table of contents from headings (H1-H6)
- Removes bold markers (`**`) from headings while preserving them in content
- Supports tables, code blocks, links, and other Markdown features
- Handles horizontal rules (`---`)
- Generates companion CSS and JavaScript files
- Responsive design that works on mobile and desktop
- Interactive TOC with smooth scrolling and active section highlighting

## Installation

1. Ensure you have a C++17 compatible compiler installed
2. Clone this repository or download the source code
3. Compile the program:

```bash
g++ -std=c++17 m2h.cpp -o m2h
```

## Usage

```bash
./m2h -m input.md -o output.html
```

Or with long options:

```bash
./m2h --markdown input.md --output output.html
```

This will generate three files:
- `output.html` - The main HTML file
- `styles.css` - The stylesheet
- `script.js` - The JavaScript for interactive features

## Options

| Option | Description |
|--------|-------------|
| `-m, --markdown` | Path to input Markdown file |
| `-o, --output` | Path to output HTML file |
| `-h, --help` | Show help message |

## Markdown Features Supported

- Headings (#, ##, etc.)
- Bold (`**text**`) and italic (`*text*`)
- Inline code (`` `code` ``)
- Code blocks (```)
- Links (`[text](url)`)
- Tables
- Horizontal rules (`---`)
- Paragraphs

## Technical Details

The converter:
1. Parses the Markdown file line by line
2. Generates IDs for each heading
3. Creates a table of contents with proper nesting
4. Converts Markdown syntax to HTML
5. Generates companion CSS and JS files
6. Produces a responsive layout with the TOC on the left

## Limitations

- Nested lists are not fully supported
- Complex table formatting may not render perfectly
- Some edge cases in Markdown parsing may not be handled

