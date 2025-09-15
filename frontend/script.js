class HeightMonitor {
    constructor() {
        this.apiBase = '/api';
        this.autoRefreshInterval = null;
        this.isAutoRefreshEnabled = true;
        this.refreshIntervalMs = 2000; // 2 seconds
        
        this.initializeElements();
        this.bindEvents();
        this.startAutoRefresh();
        this.checkServerHealth();
    }

    initializeElements() {
        // Current reading elements
        this.currentHeightEl = document.getElementById('currentHeight');
        this.connectionStatusEl = document.getElementById('connectionStatus');
        this.lastUpdatedEl = document.getElementById('lastUpdated');
        
        // Statistics elements
        this.minHeightEl = document.getElementById('minHeight');
        this.maxHeightEl = document.getElementById('maxHeight');
        this.avgHeightEl = document.getElementById('avgHeight');
        this.totalReadingsEl = document.getElementById('totalReadings');
        
        // Controls
        this.refreshBtn = document.getElementById('refreshBtn');
        this.clearBtn = document.getElementById('clearBtn');
        this.toggleAutoRefreshBtn = document.getElementById('toggleAutoRefresh');
        
        // Table
        this.readingsBody = document.getElementById('readingsBody');
        
        // Server status
        this.serverStatusEl = document.getElementById('serverStatus');
    }

    bindEvents() {
        this.refreshBtn.addEventListener('click', () => this.refreshData());
        this.clearBtn.addEventListener('click', () => this.clearHistory());
        this.toggleAutoRefreshBtn.addEventListener('click', () => this.toggleAutoRefresh());
    }

    async refreshData() {
        try {
            await Promise.all([
                this.fetchCurrentHeight(),
                this.fetchRecentReadings()
            ]);
            this.updateConnectionStatus('connected');
        } catch (error) {
            console.error('Error refreshing data:', error);
            this.updateConnectionStatus('disconnected');
        }
    }

    async fetchCurrentHeight() {
        try {
            const response = await fetch(`${this.apiBase}/height/current`);
            const data = await response.json();
            
            if (data.height !== null && data.height !== undefined) {
                this.updateCurrentHeight(data.height);
                this.updateLastUpdated(data.received_at || data.timestamp);
            } else {
                this.updateCurrentHeight('--');
                this.updateLastUpdated(null);
            }
        } catch (error) {
            throw new Error('Failed to fetch current height: ' + error.message);
        }
    }

    async fetchRecentReadings() {
        try {
            const response = await fetch(`${this.apiBase}/height?limit=20`);
            const data = await response.json();
            
            this.updateReadingsTable(data.readings || []);
            this.updateStatistics(data.readings || []);
            this.totalReadingsEl.textContent = data.total || 0;
        } catch (error) {
            throw new Error('Failed to fetch recent readings: ' + error.message);
        }
    }

    async clearHistory() {
        if (!confirm('Are you sure you want to clear all readings?')) {
            return;
        }

        try {
            const response = await fetch(`${this.apiBase}/height`, {
                method: 'DELETE'
            });
            
            if (response.ok) {
                this.resetDisplay();
                alert('History cleared successfully!');
                await this.refreshData();
            } else {
                throw new Error('Failed to clear history');
            }
        } catch (error) {
            console.error('Error clearing history:', error);
            alert('Failed to clear history. Please try again.');
        }
    }

    updateCurrentHeight(height) {
        if (height === '--' || height === null) {
            this.currentHeightEl.textContent = '--';
        } else {
            this.currentHeightEl.textContent = parseFloat(height).toFixed(1);
            this.currentHeightEl.classList.add('updating');
            setTimeout(() => {
                this.currentHeightEl.classList.remove('updating');
            }, 500);
        }
    }

    updateLastUpdated(timestamp) {
        if (!timestamp) {
            this.lastUpdatedEl.textContent = 'Last updated: Never';
            return;
        }

        const date = new Date(timestamp);
        const now = new Date();
        const diffMs = now - date;
        const diffSecs = Math.floor(diffMs / 1000);
        
        let timeAgo;
        if (diffSecs < 60) {
            timeAgo = `${diffSecs} seconds ago`;
        } else if (diffSecs < 3600) {
            timeAgo = `${Math.floor(diffSecs / 60)} minutes ago`;
        } else {
            timeAgo = date.toLocaleString();
        }
        
        this.lastUpdatedEl.textContent = `Last updated: ${timeAgo}`;
    }

    updateConnectionStatus(status) {
        this.connectionStatusEl.className = `status ${status}`;
        this.connectionStatusEl.textContent = status === 'connected' ? 'Connected' : 'Disconnected';
    }

    updateReadingsTable(readings) {
        if (!readings || readings.length === 0) {
            this.readingsBody.innerHTML = '<tr><td colspan="3" class="no-data">No readings available</td></tr>';
            return;
        }

        const rows = readings.reverse().map(reading => {
            const date = new Date(reading.received_at || reading.timestamp);
            const timeStr = date.toLocaleTimeString();
            const dateStr = date.toLocaleDateString();
            
            return `
                <tr>
                    <td>${parseFloat(reading.height).toFixed(1)}</td>
                    <td>${timeStr}<br><small>${dateStr}</small></td>
                    <td>${reading.sensor_id || 'Unknown'}</td>
                </tr>
            `;
        }).join('');

        this.readingsBody.innerHTML = rows;
    }

    updateStatistics(readings) {
        if (!readings || readings.length === 0) {
            this.minHeightEl.textContent = '--';
            this.maxHeightEl.textContent = '--';
            this.avgHeightEl.textContent = '--';
            return;
        }

        const heights = readings.map(r => parseFloat(r.height));
        const min = Math.min(...heights);
        const max = Math.max(...heights);
        const avg = heights.reduce((sum, height) => sum + height, 0) / heights.length;

        this.minHeightEl.textContent = min.toFixed(1);
        this.maxHeightEl.textContent = max.toFixed(1);
        this.avgHeightEl.textContent = avg.toFixed(1);
    }

    toggleAutoRefresh() {
        this.isAutoRefreshEnabled = !this.isAutoRefreshEnabled;
        
        if (this.isAutoRefreshEnabled) {
            this.startAutoRefresh();
            this.toggleAutoRefreshBtn.textContent = 'Auto Refresh: ON';
            this.toggleAutoRefreshBtn.setAttribute('data-active', 'true');
        } else {
            this.stopAutoRefresh();
            this.toggleAutoRefreshBtn.textContent = 'Auto Refresh: OFF';
            this.toggleAutoRefreshBtn.setAttribute('data-active', 'false');
        }
    }

    startAutoRefresh() {
        if (this.autoRefreshInterval) {
            clearInterval(this.autoRefreshInterval);
        }
        
        if (this.isAutoRefreshEnabled) {
            this.autoRefreshInterval = setInterval(() => {
                this.refreshData();
            }, this.refreshIntervalMs);
        }
    }

    stopAutoRefresh() {
        if (this.autoRefreshInterval) {
            clearInterval(this.autoRefreshInterval);
            this.autoRefreshInterval = null;
        }
    }

    async checkServerHealth() {
        try {
            const response = await fetch(`${this.apiBase}/health`);
            const data = await response.json();
            
            if (data.status === 'healthy') {
                this.serverStatusEl.textContent = 'Online';
                this.serverStatusEl.style.color = '#28a745';
            } else {
                this.serverStatusEl.textContent = 'Degraded';
                this.serverStatusEl.style.color = '#ffc107';
            }
        } catch (error) {
            this.serverStatusEl.textContent = 'Offline';
            this.serverStatusEl.style.color = '#dc3545';
        }
    }

    resetDisplay() {
        this.currentHeightEl.textContent = '--';
        this.lastUpdatedEl.textContent = 'Last updated: Never';
        this.minHeightEl.textContent = '--';
        this.maxHeightEl.textContent = '--';
        this.avgHeightEl.textContent = '--';
        this.totalReadingsEl.textContent = '0';
        this.readingsBody.innerHTML = '<tr><td colspan="3" class="no-data">No readings available</td></tr>';
        this.updateConnectionStatus('disconnected');
    }
}

// Initialize the application when the DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    window.heightMonitor = new HeightMonitor();
});

// Handle page visibility changes to pause/resume auto-refresh
document.addEventListener('visibilitychange', () => {
    if (window.heightMonitor) {
        if (document.hidden) {
            window.heightMonitor.stopAutoRefresh();
        } else if (window.heightMonitor.isAutoRefreshEnabled) {
            window.heightMonitor.startAutoRefresh();
            window.heightMonitor.refreshData();
        }
    }
});