#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

struct TocEntry {
    int level;
    std::string text;
    std::string id;
};

void show_help() {
    std::cout << "Uso: m2h -m archivo.md -o salida.html\n";
    std::cout << "Convierte archivos Markdown a HTML con tabla de contenidos\n\n";
    std::cout << "Opciones:\n";
    std::cout << "  -m, --markdown <archivo>  Archivo Markdown de entrada\n";
    std::cout << "  -o, --output <archivo>    Archivo HTML de salida\n";
    std::cout << "  -h, --help                Muestra este mensaje de ayuda\n";
}

bool parse_args(int argc, char** argv, fs::path& markdown_path, fs::path& output_path) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            show_help();
            return false;
        }
        else if ((arg == "-m" || arg == "--markdown") && i + 1 < argc) {
            markdown_path = argv[++i];
        }
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            output_path = argv[++i];
        }
    }
    
    if (markdown_path.empty() || output_path.empty()) {
        std::cerr << "Error: Se requieren tanto el archivo de entrada como el de salida\n";
        show_help();
        return false;
    }
    
    return true;
}

std::string sanitize_id(const std::string& text) {
    std::string id = text;
    
    // Convertir a minúsculas
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    
    // Reemplazar espacios con guiones
    std::replace(id.begin(), id.end(), ' ', '-');
    
    // Eliminar caracteres no permitidos
    id.erase(std::remove_if(id.begin(), id.end(), [](char c) {
        return !(isalnum(c) || c == '-' || c == '_');
    }), id.end());
    
    return id;
}

std::string generate_unique_id(const std::string& base_id, std::map<std::string, int>& id_counts) {
    std::string id = sanitize_id(base_id);
    
    if (id_counts.find(id) != id_counts.end()) {
        int count = ++id_counts[id];
        return id + "-" + std::to_string(count);
    } else {
        id_counts[id] = 0;
        return id;
    }
}

std::string clean_header_text(std::string text) {
    // Eliminar marcadores **
    size_t pos = 0;
    while ((pos = text.find("**", pos)) != std::string::npos) {
        text.erase(pos, 2);
    }
    return text;
}

std::string process_table_line(const std::string& line) {
    std::string processed = line;
    if (!processed.empty() && processed.front() == '|') processed.erase(0, 1);
    if (!processed.empty() && processed.back() == '|') processed.pop_back();
    return processed;
}

std::string markdown_to_html(const std::string& markdown, std::vector<TocEntry>& toc) {
    std::string html;
    std::istringstream iss(markdown);
    std::string line;
    
    std::regex h1_regex("^#\\s+(.*)");
    std::regex h2_regex("^##\\s+(.*)");
    std::regex h3_regex("^###\\s+(.*)");
    std::regex h4_regex("^####\\s+(.*)");
    std::regex h5_regex("^#####\\s+(.*)");
    std::regex h6_regex("^######\\s+(.*)");
    std::regex code_block_regex("^```(.*)");
    std::regex link_regex("\\[([^\\]]+)\\]\\(([^\\)]+)\\)");
    std::regex bold_regex("\\*\\*([^\\*]+)\\*\\*");
    std::regex italic_regex("\\*([^\\*]+)\\*");
    std::regex inline_code_regex("`([^`]+)`");
    std::regex table_divider_regex("^\\s*\\|?[-:]+\\|[-:\\s\\|]+\\|?\\s*$");
    std::regex horizontal_rule_regex("^\\s*[-*_]{3,}\\s*$");
    
    bool in_code_block = false;
    bool in_table = false;
    std::string code_lang;
    std::vector<std::vector<std::string>> table_rows;
    std::map<std::string, int> id_counts;
    
    while (std::getline(iss, line)) {
        std::smatch match;
        
        // Ignorar líneas vacías
        if (line.empty()) continue;
        
        // Manejar reglas horizontales (---)
        if (std::regex_match(line, horizontal_rule_regex)) {
            html += "<hr>\n";
            continue;
        }
        
        // Manejar bloques de código
        if (std::regex_match(line, match, code_block_regex)) {
            if (!in_code_block) {
                in_code_block = true;
                code_lang = match[1].str();
                html += "<pre><code" + (code_lang.empty() ? "" : " class=\"language-" + code_lang + "\"") + ">\n";
                continue;
            } else {
                in_code_block = false;
                html += "</code></pre>\n";
                continue;
            }
        }
        
        if (in_code_block) {
            html += line + "\n";
            continue;
        }
        
        // Manejar tablas
        if (std::regex_match(line, table_divider_regex)) {
            if (!in_table && !table_rows.empty()) {
                in_table = true;
                continue;
            }
        } else if (line.find('|') != std::string::npos) {
            std::string processed_line = process_table_line(line);
            std::istringstream row_stream(processed_line);
            std::string cell;
            std::vector<std::string> row_cells;
            
            while (std::getline(row_stream, cell, '|')) {
                cell.erase(0, cell.find_first_not_of(" \t"));
                cell.erase(cell.find_last_not_of(" \t") + 1);
                
                cell = std::regex_replace(cell, bold_regex, "<strong>$1</strong>");
                cell = std::regex_replace(cell, italic_regex, "<em>$1</em>");
                cell = std::regex_replace(cell, inline_code_regex, "<code>$1</code>");
                cell = std::regex_replace(cell, link_regex, "<a href=\"$2\">$1</a>");
                
                row_cells.push_back(cell);
            }
            
            table_rows.push_back(row_cells);
            continue;
        } else if (in_table) {
            html += "<table>\n";
            bool first_row = true;
            for (const auto& row : table_rows) {
                if (row.empty()) continue;
                
                html += "<tr>";
                for (const auto& cell : row) {
                    std::string tag = first_row ? "th" : "td";
                    html += "<" + tag + ">" + cell + "</" + tag + ">";
                }
                html += "</tr>\n";
                first_row = false;
            }
            html += "</table>\n";
            table_rows.clear();
            in_table = false;
        }
        
        // Manejar encabezados
        if (std::regex_match(line, match, h1_regex)) {
            std::string text = clean_header_text(match[1].str());
            std::string id = generate_unique_id(text, id_counts);
            toc.push_back({1, text, id});
            html += "<h1 id=\"" + id + "\">" + text + "</h1>\n";
            continue;
        } else if (std::regex_match(line, match, h2_regex)) {
            std::string text = clean_header_text(match[1].str());
            std::string id = generate_unique_id(text, id_counts);
            toc.push_back({2, text, id});
            html += "<h2 id=\"" + id + "\">" + text + "</h2>\n";
            continue;
        } else if (std::regex_match(line, match, h3_regex)) {
            std::string text = clean_header_text(match[1].str());
            std::string id = generate_unique_id(text, id_counts);
            toc.push_back({3, text, id});
            html += "<h3 id=\"" + id + "\">" + text + "</h3>\n";
            continue;
        } else if (std::regex_match(line, match, h4_regex)) {
            std::string text = clean_header_text(match[1].str());
            std::string id = generate_unique_id(text, id_counts);
            toc.push_back({4, text, id});
            html += "<h4 id=\"" + id + "\">" + text + "</h4>\n";
            continue;
        } else if (std::regex_match(line, match, h5_regex)) {
            std::string text = clean_header_text(match[1].str());
            std::string id = generate_unique_id(text, id_counts);
            toc.push_back({5, text, id});
            html += "<h5 id=\"" + id + "\">" + text + "</h5>\n";
            continue;
        } else if (std::regex_match(line, match, h6_regex)) {
            std::string text = clean_header_text(match[1].str());
            std::string id = generate_unique_id(text, id_counts);
            toc.push_back({6, text, id});
            html += "<h6 id=\"" + id + "\">" + text + "</h6>\n";
            continue;
        }
        
        // Procesar otros elementos markdown
        std::string processed_line = line;
        processed_line = std::regex_replace(processed_line, link_regex, "<a href=\"$2\">$1</a>");
        processed_line = std::regex_replace(processed_line, bold_regex, "<strong>$1</strong>");
        processed_line = std::regex_replace(processed_line, italic_regex, "<em>$1</em>");
        processed_line = std::regex_replace(processed_line, inline_code_regex, "<code>$1</code>");
        
        if (!processed_line.empty()) {
            html += "<p>" + processed_line + "</p>\n";
        }
    }
    
    // Manejar tabla al final del archivo
    if (in_table && !table_rows.empty()) {
        html += "<table>\n";
        bool first_row = true;
        for (const auto& row : table_rows) {
            if (row.empty()) continue;
            
            html += "<tr>";
            for (const auto& cell : row) {
                std::string tag = first_row ? "th" : "td";
                html += "<" + tag + ">" + cell + "</" + tag + ">";
            }
            html += "</tr>\n";
            first_row = false;
        }
        html += "</table>\n";
    }
    
    return html;
}

std::string generate_toc_html(const std::vector<TocEntry>& toc) {
    if (toc.empty()) return "";
    
    std::string html = "<div class=\"toc\">\n<h2>Table of Contents</h2>\n<ul>\n";
    int current_level = 1;
    
    for (const auto& entry : toc) {
        while (current_level < entry.level) {
            html += "<ul>\n";
            current_level++;
        }
        
        while (current_level > entry.level) {
            html += "</ul>\n";
            current_level--;
        }
        
        html += "<li><a href=\"#" + entry.id + "\" data-id=\"" + entry.id + "\">" + entry.text + "</a></li>\n";
    }
    
    while (current_level > 1) {
        html += "</ul>\n";
        current_level--;
    }
    
    html += "</ul>\n</div>\n";
    return html;
}

std::string generate_css() {
    return R"(
body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    line-height: 1.6;
    color: #333;
    margin: 0;
    padding: 0;
    display: flex;
    min-height: 100vh;
}

.toc {
    width: 250px;
    background-color: #f5f5f5;
    padding: 20px;
    border-right: 1px solid #ddd;
    position: fixed;
    height: 100vh;
    overflow-y: auto;
    box-sizing: border-box;
}

.toc h2 {
    margin-top: 0;
    font-size: 1.2em;
    color: #444;
}

.toc ul {
    list-style: none;
    padding-left: 15px;
    margin: 0;
}

.toc li {
    margin: 5px 0;
    position: relative;
}

.toc a {
    color: #0366d6;
    text-decoration: none;
    display: block;
    padding: 2px 0;
    transition: all 0.2s ease;
}

.toc a:hover {
    text-decoration: underline;
}

.toc a.active {
    font-weight: bold;
    color: #d03636;
    border-left: 3px solid #d03636;
    padding-left: 8px;
    margin-left: -11px;
    background-color: #f9f9f9;
}

.content {
    flex: 1;
    margin-left: 290px;
    padding: 20px;
    max-width: 800px;
}

h1, h2, h3, h4, h5, h6 {
    color: #222;
    margin-top: 1.5em;
    margin-bottom: 0.5em;
    font-weight: 600;
    scroll-margin-top: 20px;
}

h1 { font-size: 2em; border-bottom: 1px solid #eaecef; padding-bottom: 0.3em; }
h2 { font-size: 1.5em; border-bottom: 1px solid #eaecef; padding-bottom: 0.3em; }
h3 { font-size: 1.25em; }
h4 { font-size: 1em; }
h5 { font-size: 0.875em; }
h6 { font-size: 0.85em; color: #6a737d; }

pre {
    background-color: #f6f8fa;
    border-radius: 3px;
    padding: 16px;
    overflow: auto;
}

code {
    font-family: SFMono-Regular, Consolas, "Liberation Mono", Menlo, monospace;
    background-color: rgba(27,31,35,0.05);
    border-radius: 3px;
    padding: 0.2em 0.4em;
    font-size: 85%;
}

pre code {
    background-color: transparent;
    padding: 0;
}

table {
    border-collapse: collapse;
    width: 100%;
    margin: 1em 0;
}

th, td {
    border: 1px solid #ddd;
    padding: 8px;
    text-align: left;
}

th {
    background-color: #f2f2f2;
    font-weight: bold;
}

tr:nth-child(even) {
    background-color: #f9f9f9;
}

a {
    color: #0366d6;
    text-decoration: none;
}

a:hover {
    text-decoration: underline;
}

hr {
    border: 0;
    height: 1px;
    background-color: #ddd;
    margin: 1.5em 0;
}

@media (max-width: 768px) {
    body {
        flex-direction: column;
    }
    
    .toc {
        width: 100%;
        height: auto;
        position: relative;
        border-right: none;
        border-bottom: 1px solid #ddd;
    }
    
    .content {
        margin-left: 0;
        padding: 20px;
    }
}
)";
}

std::string generate_js() {
    return R"(
document.addEventListener('DOMContentLoaded', function() {
    // Configuración del IntersectionObserver
    const observer = new IntersectionObserver(function(entries) {
        entries.forEach(function(entry) {
            if (entry.isIntersecting) {
                const id = entry.target.getAttribute('id');
                const tocLink = document.querySelector(`.toc a[href="#${id}"]`);
                
                // Remover activo de todos los enlaces
                document.querySelectorAll('.toc a').forEach(function(link) {
                    link.classList.remove('active');
                });
                
                // Agregar activo al enlace actual
                if (tocLink) {
                    tocLink.classList.add('active');
                    
                    // Scroll suave para mostrar el enlace activo
                    const toc = document.querySelector('.toc');
                    const linkRect = tocLink.getBoundingClientRect();
                    const tocRect = toc.getBoundingClientRect();
                    
                    if (linkRect.top < tocRect.top) {
                        toc.scrollTo({
                            top: toc.scrollTop + linkRect.top - tocRect.top - 20,
                            behavior: 'smooth'
                        });
                    } else if (linkRect.bottom > tocRect.bottom) {
                        toc.scrollTo({
                            top: toc.scrollTop + linkRect.bottom - tocRect.bottom + 20,
                            behavior: 'smooth'
                        });
                    }
                }
            }
        });
    }, {
        root: null,
        rootMargin: '0px',
        threshold: 0.5
    });

    // Observar todos los encabezados
    document.querySelectorAll('h1, h2, h3, h4, h5, h6').forEach(function(header) {
        observer.observe(header);
    });

    // Scroll suave para enlaces del TOC
    document.querySelectorAll('.toc a').forEach(function(link) {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            const targetId = this.getAttribute('href');
            const targetElement = document.querySelector(targetId);
            
            if (targetElement) {
                // Scroll suave al elemento
                targetElement.scrollIntoView({
                    behavior: 'smooth',
                    block: 'start'
                });
                
                // Actualizar URL
                history.pushState(null, null, targetId);
            }
        });
    });
});
)";
}

std::string generate_html_template(const std::string& title, const std::string& toc_html, const std::string& content_html) {
    return R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)" + title + R"(</title>
    <link rel="stylesheet" href="styles.css">
</head>
<body>
)" + toc_html + R"(
    <div class="content">
)" + content_html + R"(
    </div>
    <script src="script.js"></script>
</body>
</html>
)";
}

bool write_file(const fs::path& path, const std::string& content) {
    std::ofstream out(path);
    if (!out) {
        std::cerr << "Error: Could not open file " << path << " for writing\n";
        return false;
    }
    out << content;
    return true;
}

int main(int argc, char** argv) {
    fs::path markdown_path;
    fs::path output_path;
    
    if (!parse_args(argc, argv, markdown_path, output_path)) {
        return 1;
    }
    
    if (!fs::exists(markdown_path)) {
        std::cerr << "Error: El archivo de entrada no existe: " << markdown_path << "\n";
        return 1;
    }
    
    std::ifstream md_file(markdown_path);
    if (!md_file) {
        std::cerr << "Error: Could not open input file " << markdown_path << "\n";
        return 1;
    }
    
    std::string markdown_content((std::istreambuf_iterator<char>(md_file)), 
                               std::istreambuf_iterator<char>());
    
    std::vector<TocEntry> toc;
    std::string html_content = markdown_to_html(markdown_content, toc);
    std::string toc_html = generate_toc_html(toc);
    
    fs::path output_dir = output_path.parent_path();
    if (!output_dir.empty() && !fs::exists(output_dir)) {
        fs::create_directories(output_dir);
    }
    
    std::string title = toc.empty() ? "Document" : toc[0].text;
    std::string html = generate_html_template(title, toc_html, html_content);
    
    if (!write_file(output_path, html)) {
        return 1;
    }
    
    fs::path css_path = output_dir / "styles.css";
    if (!write_file(css_path, generate_css())) {
        return 1;
    }
    
    fs::path js_path = output_dir / "script.js";
    if (!write_file(js_path, generate_js())) {
        return 1;
    }
    
    std::cout << "Successfully generated:\n";
    std::cout << "  HTML: " << output_path << "\n";
    std::cout << "  CSS: " << css_path << "\n";
    std::cout << "  JS: " << js_path << "\n";
    
    return 0;
}
