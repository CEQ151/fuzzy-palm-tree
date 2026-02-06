class FileManagerApp {
    constructor() {
        this.apiBaseUrl = window.location.origin;
        this.currentFiles = [];
        this.currentPath = '';
        this.totalSize = 0;
        
        this.initElements();
        this.initEventListeners();
        this.checkServerStatus();
        this.loadState(); // Load saved state on startup
    }
    
    initElements() {
        // Settings elements
        this.showSizeCheckbox = document.getElementById('show-size');
        // humanReadableCheckbox removed
        this.maxDepthInput = document.getElementById('max-depth');
        this.excludePatternsInput = document.getElementById('exclude-patterns');
        
        // Info elements
        this.currentPathElement = document.getElementById('current-path');
        this.fileCountElement = document.getElementById('file-count');
        this.totalSizeElement = document.getElementById('total-size');
        
        // Action buttons
        this.scanBtn = document.getElementById('scan-btn');
        this.generateTreeBtn = document.getElementById('generate-tree-btn');
        this.downloadBtn = document.getElementById('download-btn');
        this.browseBtn = document.getElementById('browse-btn');
        this.copyBtn = document.getElementById('copy-btn');
        this.clearBtn = document.getElementById('clear-btn'); // Only clears tree view
        this.clearAllBtn = document.getElementById('clear-all-btn'); // Clears everything
        
        // Input elements
        this.directoryPathInput = document.getElementById('directory-path');
        
        // Output elements
        this.treeOutput = document.getElementById('tree-output');
        this.fileTableBody = document.getElementById('file-table-body');
        
        // Status elements
        this.serverStatusElement = document.getElementById('server-status');
        this.apiStatusElement = document.getElementById('api-status');
    }
    
    initEventListeners() {
        // Button events
        this.scanBtn.addEventListener('click', () => this.scanDirectory(true));
        
        // "Generate Tree" now effectively means "Apply Settings & Regenerate"
        // Since filtering happens at scan time, we need to rescan to apply depth/excludes
        this.generateTreeBtn.addEventListener('click', () => this.scanDirectory(true));
        
        this.downloadBtn.addEventListener('click', () => this.downloadTree());
        this.browseBtn.addEventListener('click', () => this.browseDirectory());
        this.copyBtn.addEventListener('click', () => this.copyTreeToClipboard());
        this.clearBtn.addEventListener('click', () => this.clearResults());
        this.clearAllBtn.addEventListener('click', () => this.clearAllData());
        
        // Input events
        this.directoryPathInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                this.scanDirectory(true);
            }
        });

        // Save settings when changed
        [this.showSizeCheckbox, this.maxDepthInput, this.excludePatternsInput].forEach(el => {
            el.addEventListener('change', () => this.saveState());
        });
    }
    
    // Upload functions removed
    
    async scanDirectory(autoGenerate = true) {
        const path = this.directoryPathInput.value.trim();
        
        if (!path) {
            this.showToast('Please enter a directory path', 'warning');
            return;
        }
        
        this.showToast('Scanning directory...', 'info');
        
        try {
            const options = this.getTreeOptions();
            
            const response = await fetch(`${this.apiBaseUrl}/api/scan`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    path: path,
                    ...options
                })
            });
            
            const result = await response.json();
            
            if (result.success) {
                this.currentFiles = result.files || [];
                this.currentPath = result.path;
                
                // Update UI
                this.currentPathElement.textContent = this.currentPath;
                this.fileCountElement.textContent = result.file_count || 0;
                
                // Calculate total size
                this.totalSize = this.currentFiles.reduce((sum, file) => sum + (file.size || 0), 0);
                this.totalSizeElement.textContent = this.formatFileSize(this.totalSize);
                
                // Update file table
                this.updateFileTable();
                
                this.showToast(`Found ${result.file_count} files/directories`, 'success');
                
                // Auto generate tree if requested
                if (autoGenerate) {
                    await this.generateTree();
                }

                this.saveState();
            } else {
                this.showToast(`Scan failed: ${result.message}`, 'error');
            }
        } catch (error) {
            this.showToast(`Scan error: ${error.message}`, 'error');
        }
    }
    
    async generateTree() {
        if (this.currentFiles.length === 0) {
            // If no files, try scanning first
            if (this.directoryPathInput.value.trim()) {
                await this.scanDirectory(false); // prevent infinite loop
                if (this.currentFiles.length === 0) return;
            } else {
                this.showToast('Please scan a directory first', 'warning');
                return;
            }
        }
        
        try {
            // Generate tree on client side
            const treeHtml = this.generateTreeContent(true); // HTML format for display
            
            // For saving state and clipboard, we might want plain text
            // But for display, we use HTML
            this.treeOutput.innerHTML = treeHtml;
            this.saveState();
            // this.showToast('File tree generated successfully!', 'success');
        } catch (error) {
            this.showToast(`Tree generation error: ${error.message}`, 'error');
            console.error(error);
        }
    }
    
    // Generate tree content (HTML or Plain Text)
    generateTreeContent(asHtml) {
        if (this.currentFiles.length === 0) return asHtml ? '' : 'No files found.';

        let output = '';
        const files = this.currentFiles;
        
        // 1. Output Root Directory
        // Extract root name from current path
        let rootName = this.currentPath.split(/[/\\]/).filter(Boolean).pop();
        if (!rootName && this.currentPath === '.') rootName = 'root'; // Fallback
        if (!rootName) rootName = this.currentPath; // Fallback
        
        // Add trailing slash for directories
        rootName += '/';
        
        if (asHtml) {
            output += `<span class="tree-icon">📁</span> <span class="tree-item-name" style="font-weight:bold">${rootName}</span>\n`;
        } else {
            output += `${rootName}\n`;
        }
        
        // Track the "is last child" status for each depth level
        const isLastAtDepth = new Array(100).fill(false);
        
        // Helper to check if a file is the last child of its parent
        const checkIsLast = (index, currentDepth) => {
            for (let i = index + 1; i < files.length; i++) {
                const nextFile = files[i];
                if (nextFile.depth < currentDepth) return true; // Back up a level, so we were last
                if (nextFile.depth === currentDepth) return false; // Found a sibling
            }
            return true; // End of list
        };

        for (let i = 0; i < files.length; i++) {
            const file = files[i];
            
            // Determine if this is the last item at this level
            const isLast = checkIsLast(i, file.depth);
            isLastAtDepth[file.depth] = isLast;

            // Indentation (start from depth 1)
            // If file.depth is 1, loop doesn't run -> no indentation prefix before connector
            for (let d = 1; d < file.depth; d++) {
                output += isLastAtDepth[d] ? '    ' : '│   ';
            }

            // Connector
            output += isLast ? '└── ' : '├── ';

            // Icon and Name
            const iconChar = file.is_directory ? '📁' : '📄';
            let fileName = file.name;
            if (file.is_directory) fileName += '/'; // Add slash for dirs
            
            if (asHtml) {
                output += `<span class="tree-icon">${iconChar}</span> <span class="tree-item-name">${fileName}</span>`;
            } else {
                output += `${fileName}`; // Text mode: no icon, just name (or maybe with icon if requested, user asked for clean format)
                // User's example has no icons in text mode, but maybe folders have slash
            }

            // Size (optional)
            if (this.showSizeCheckbox.checked) {
                const sizeStr = this.formatFileSize(file.size || 0);
                output += ` (${sizeStr})`;
            }

            output += '\n';
        }

        return output;
    }

    // getIconHtml removed as we use simple emojis directly

    async downloadTree() {
        if (this.currentFiles.length === 0) {
            this.showToast('Please scan a directory first', 'warning');
            return;
        }
        
        try {
            // Generate plain text tree for download
            const treeText = this.generateTreeContent(false);
            
            const blob = new Blob([treeText], { type: 'text/plain' });
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `file_tree_${new Date().getTime()}.txt`;
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
            document.body.removeChild(a);
            
            this.showToast('Download started', 'success');
        } catch (error) {
            this.showToast(`Download error: ${error.message}`, 'error');
        }
    }
    
    // browseDirectory() { ... (unchanged)

    async copyTreeToClipboard() {
        // Use plain text for clipboard
        const treeText = this.generateTreeContent(false);
        
        if (!treeText || treeText === 'No files found.') {
            this.showToast('No tree to copy', 'warning');
            return;
        }
        
        try {
            await navigator.clipboard.writeText(treeText);
            this.showToast('Tree copied to clipboard!', 'success');
        } catch (error) {
            this.showToast(`Failed to copy: ${error.message}`, 'error');
        }
    }
    
    clearResults() {
        this.treeOutput.innerHTML = 'Select a folder and click "Generate Tree" to see the file structure here.';
        this.currentFiles = [];
        this.currentPath = '';
        this.totalSize = 0;
        
        // Update UI
        this.currentPathElement.textContent = 'No folder selected';
        this.fileCountElement.textContent = '0';
        this.totalSizeElement.textContent = '0 B';
        
        // Clear file table
        this.fileTableBody.innerHTML = '<tr><td colspan="4" class="empty-message">No files scanned yet</td></tr>';
        
        this.showToast('Results cleared', 'success');
    }
    
    clearAllData() {
        if (!confirm('Are you sure you want to clear all data and settings?')) return;
        
        localStorage.removeItem('fmg_state');
        this.clearResults();
        this.directoryPathInput.value = '';
        
        // Reset settings to defaults
        this.showSizeCheckbox.checked = true;
        // humanReadableCheckbox removed
        this.maxDepthInput.value = -1;
        this.excludePatternsInput.value = '';
        
        this.showToast('All data cleared', 'success');
    }
    
    getTreeOptions() {
        const excludePatterns = this.excludePatternsInput.value
            .split(',')
            .map(pattern => pattern.trim())
            .filter(pattern => pattern.length > 0);
        
        return {
            show_size: this.showSizeCheckbox.checked,
            human_readable: true, // Always true
            max_depth: parseInt(this.maxDepthInput.value) || -1,
            exclude_patterns: excludePatterns
        };
    }
    
    getFileIcon(filename, isDirectory) {
        if (isDirectory) return '📁';
        return '📄';
    }

    updateFileTable() {
        if (this.currentFiles.length === 0) {
            this.fileTableBody.innerHTML = '<tr><td colspan="4" class="empty-message">No files scanned yet</td></tr>';
            return;
        }
        
        let html = '';
        
        this.currentFiles.forEach(file => {
            const type = file.is_directory ? 'Directory' : 'File';
            const typeClass = file.is_directory ? 'directory' : 'file';
            const size = this.formatFileSize(file.size || 0);
            const icon = this.getFileIcon(file.name, file.is_directory);
            
            html += `
                <tr>
                    <td>
                        <div class="file-name-cell">
                            <span class="file-icon">${icon}</span>
                            <span class="name-text" title="${file.name}">${file.name}</span>
                        </div>
                    </td>
                    <td><span class="type-badge ${typeClass}">${type}</span></td>
                    <td>${size}</td>
                    <td>${file.depth}</td>
                </tr>
            `;
        });
        
        this.fileTableBody.innerHTML = html;
    }
    
    formatFileSize(bytes) {
        if (bytes === 0) return '0 B';
        
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }
    
    async checkServerStatus() {
        try {
            const response = await fetch(`${this.apiBaseUrl}/api/info`);
            if (response.ok) {
                const data = await response.json();
                this.serverStatusElement.textContent = 'Connected';
                this.apiStatusElement.textContent = `API: v${data.version}`;
                this.showToast('Server connected successfully', 'success');
            } else {
                this.serverStatusElement.textContent = 'Disconnected';
                this.apiStatusElement.textContent = 'API: Not available';
                this.showToast('Server not responding', 'error');
            }
        } catch (error) {
            this.serverStatusElement.textContent = 'Disconnected';
            this.apiStatusElement.textContent = 'API: Not available';
            this.showToast('Cannot connect to server', 'error');
        }
    }
    
    showToast(message, type = 'info') {
        const toastContainer = document.getElementById('toast-container');
        
        const toast = document.createElement('div');
        toast.className = `toast ${type}`;
        
        let icon = 'info-circle';
        if (type === 'success') icon = 'check-circle';
        if (type === 'error') icon = 'exclamation-circle';
        if (type === 'warning') icon = 'exclamation-triangle';
        
        toast.innerHTML = `
            <i class="fas fa-${icon}"></i>
            <span>${message}</span>
        `;
        
        toastContainer.appendChild(toast);
        
        // Remove toast after 5 seconds
        setTimeout(() => {
            toast.style.animation = 'slideIn 0.3s ease reverse';
            setTimeout(() => {
                if (toast.parentNode) {
                    toast.parentNode.removeChild(toast);
                }
            }, 300);
        }, 5000);
    }

    saveState() {
        const state = {
            path: this.directoryPathInput.value,
            settings: this.getTreeOptions(),
            // Don't save tree output HTML, regenerate it from files
            currentFiles: this.currentFiles,
            currentPath: this.currentPath,
            totalSize: this.totalSize
        };
        localStorage.setItem('fmg_state', JSON.stringify(state));
    }

    loadState() {
        const saved = localStorage.getItem('fmg_state');
        if (!saved) return;

        try {
            const state = JSON.parse(saved);
            
            if (state.path) this.directoryPathInput.value = state.path;
            
            if (state.settings) {
                this.showSizeCheckbox.checked = state.settings.show_size;
                // humanReadableCheckbox removed
                this.maxDepthInput.value = state.settings.max_depth;
                if (state.settings.exclude_patterns) {
                    this.excludePatternsInput.value = state.settings.exclude_patterns.join(', ');
                }
            }
            
            if (state.currentFiles) {
                this.currentFiles = state.currentFiles;
                this.updateFileTable();
                
                // Regenerate tree view
                if (this.currentFiles.length > 0) {
                    this.treeOutput.innerHTML = this.generateTreeContent(true);
                }
            }
            
            if (state.currentPath) this.currentPathElement.textContent = state.currentPath;
            if (state.totalSize !== undefined) {
                this.totalSize = state.totalSize;
                this.totalSizeElement.textContent = this.formatFileSize(this.totalSize);
            }
            if (state.currentFiles) {
                this.fileCountElement.textContent = state.currentFiles.length;
            }

        } catch (e) {
            console.error("Failed to load state", e);
        }
    }
}

// Initialize the app when the page loads
window.addEventListener('DOMContentLoaded', () => {
    new FileManagerApp();
});

// Add some CSS for the file table
const style = document.createElement('style');
style.textContent = `
    .type-badge {
        display: inline-block;
        padding: 3px 8px;
        border-radius: 12px;
        font-size: 0.85rem;
        font-weight: 600;
    }
    
    .type-badge.directory {
        background-color: #e3f2fd;
        color: #1976d2;
    }
    
    .type-badge.file {
        background-color: #f3e5f5;
        color: #7b1fa2;
    }
    
    .drag-over {
        background-color: rgba(67, 97, 238, 0.2) !important;
        border-color: #3a56d4 !important;
    }
    
    .file-name-cell {
        display: flex;
        align-items: center;
        gap: 10px;
        white-space: normal;
        word-break: break-all;
    }
    
    .file-name-cell i {
        /* min-width: 20px; */
        /* text-align: center; */
        /* flex-shrink: 0; */
        /* font-size: 1.1em; */
    }
    
    /* Tree View Icons */
    .tree-icon {
        margin-right: 5px;
        min-width: 16px;
        text-align: center;
        display: inline-block;
    }
    
    .tree-item-name {
        /* font-weight: 500; */
    }
`;
document.head.appendChild(style);