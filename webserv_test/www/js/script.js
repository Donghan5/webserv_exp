document.addEventListener('DOMContentLoaded', function() {
    // Simple animation for the header
    const header = document.querySelector('header');
    if (header) {
        header.style.opacity = '0';
        setTimeout(() => {
            header.style.transition = 'opacity 1s';
            header.style.opacity = '1';
        }, 100);
    }
    
    // Add event listener to links
    const links = document.querySelectorAll('a');
    links.forEach(link => {
        link.addEventListener('click', function(e) {
            console.log('Link clicked: ' + this.href);
        });
    });
    
    // Initialize any interactive elements
    initializeInteractiveElements();
});

function initializeInteractiveElements() {
    // This function can be extended to initialize any interactive elements
    // that might be added to the page
    
    // Example: Initialize any tooltips
    const tooltips = document.querySelectorAll('[data-tooltip]');
    if (tooltips.length > 0) {
        tooltips.forEach(tooltip => {
            tooltip.addEventListener('mouseenter', showTooltip);
            tooltip.addEventListener('mouseleave', hideTooltip);
        });
    }
    
    // Example: Initialize tabs if present
    const tabContainers = document.querySelectorAll('.tabs-container');
    if (tabContainers.length > 0) {
        tabContainers.forEach(container => {
            initializeTabs(container);
        });
    }
}

function showTooltip(e) {
    const tooltip = this.dataset.tooltip;
    if (!tooltip) return;
    
    const tooltipEl = document.createElement('div');
    tooltipEl.className = 'tooltip';
    tooltipEl.textContent = tooltip;
    
    document.body.appendChild(tooltipEl);
    
    const rect = this.getBoundingClientRect();
    tooltipEl.style.left = rect.left + (rect.width / 2) - (tooltipEl.offsetWidth / 2) + 'px';
    tooltipEl.style.top = rect.top - tooltipEl.offsetHeight - 10 + 'px';
    
    setTimeout(() => {
        tooltipEl.classList.add('visible');
    }, 10);
    
    this._tooltipEl = tooltipEl;
}

function hideTooltip() {
    if (this._tooltipEl) {
        this._tooltipEl.classList.remove('visible');
        setTimeout(() => {
            if (this._tooltipEl.parentNode) {
                this._tooltipEl.parentNode.removeChild(this._tooltipEl);
            }
            this._tooltipEl = null;
        }, 300);
    }
}

function initializeTabs(container) {
    const tabs = container.querySelectorAll('.tab');
    const tabContents = container.querySelectorAll('.tab-content');
    
    tabs.forEach((tab, index) => {
        tab.addEventListener('click', () => {
            // Remove active class from all tabs and contents
            tabs.forEach(t => t.classList.remove('active'));
            tabContents.forEach(content => content.classList.remove('active'));
            
            // Add active class to current tab and content
            tab.classList.add('active');
            tabContents[index].classList.add('active');
        });
    });
    
    // Activate first tab by default
    if (tabs.length > 0) {
        tabs[0].click();
    }
}