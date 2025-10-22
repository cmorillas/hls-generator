(function() {
    console.log("[hls-generator] Cookie consent killer v2 initializing...");

    // IMPROVEMENT 1: Detect modal container by high z-index
    function findModalContainer() {
        const elements = document.querySelectorAll('*');
        let bestCandidate = null;
        let highestZIndex = 1000;

        for (const el of elements) {
            const style = window.getComputedStyle(el);
            const zIndex = parseInt(style.zIndex) || 0;
            const position = style.position;

            if (zIndex > highestZIndex && (position === 'fixed' || position === 'absolute')) {
                highestZIndex = zIndex;
                bestCandidate = el;
            }
        }

        if (bestCandidate) {
            console.log('[hls-generator] Found modal container with z-index:', highestZIndex, bestCandidate);
            return bestCandidate;
        }

        return document.body; // Fallback to full document
    }

    function findAndClickCookieConsent() {
        // Multi-language keywords for cookie acceptance
        const acceptKeywords = [
            // Spanish
            'aceptar todo', 'aceptar', 'acepto', 'consentir', 'de acuerdo', 'continuar',
            // English
            'accept all', 'accept', 'i agree', 'agree', 'consent', 'got it', 'ok', 'allow all', 'continue', 'allow',
            // German
            'akzeptieren', 'zustimmen', 'einverstanden', 'alle akzeptieren',
            // French
            'accepter', 'tout accepter', "j'accepte", "d'accord",
            // Italian
            'accetto', 'accetta tutto', 'accettare', 'continua',
            // Portuguese
            'aceitar', 'aceitar tudo', 'concordo'
        ];

        // IMPROVEMENT 2: Negative keywords to avoid clicking wrong buttons
        const rejectKeywords = [
            'reject', 'decline', 'deny', 'refuse', 'customize', 'settings',
            'manage', 'preferences', 'only necessary', 'only essential',
            'rechazar', 'denegar', 'personalizar', 'configurar',
            'ablehnen', 'anpassen', 'rifiuta', 'recusar'
        ];

        const selectorPatterns = ['button', 'a', '[role="button"]', 'div[onclick]'];

        const tryClick = (element, reason) => {
            if (element && typeof element.click === 'function') {
                // Check if element is visible
                if (element.offsetParent !== null || element.checkVisibility?.() !== false) {
                    const text = element.innerText?.trim() || element.getAttribute('aria-label') || '';
                    console.log('[hls-generator] ✓ Clicking consent button:', text, '(Reason:', reason, ')');
                    element.click();
                    return true;
                }
            }
            return false;
        };

        // Find the modal container first (high z-index)
        const modalContainer = findModalContainer();

        // Search in modal first, then in whole document as fallback
        const searchContainers = [modalContainer];
        if (modalContainer !== document.body) {
            searchContainers.push(document.body); // Fallback to full document if modal search fails
        }

        // Strategy 1: Search by visible text within modal (and fallback to document)
        for (const container of searchContainers) {
            for (const selector of selectorPatterns) {
                const elements = container.querySelectorAll(selector);
                for (const el of elements) {
                    const elementText = (el.innerText?.toLowerCase().trim() || '') + ' ' +
                                       (el.getAttribute('aria-label')?.toLowerCase() || '') + ' ' +
                                       (el.getAttribute('title')?.toLowerCase() || '');

                    // Skip if contains reject keywords
                    if (rejectKeywords.some(keyword => elementText.includes(keyword))) {
                        continue;
                    }

                    // Click if contains accept keywords
                    if (acceptKeywords.some(keyword => elementText.includes(keyword))) {
                        if (tryClick(el, 'keyword match')) return true;
                    }
                }
            }
        }

        // Strategy 2: Search by attributes (id, class, aria-label) in modal and document
        for (const searchContainer of searchContainers) {
            const attributeKeywords = ['cookie', 'consent', 'banner', 'privacy', 'gdpr', 'onetrust'];
            for (const attrKeyword of attributeKeywords) {
                const selectors = [
                    `[id*="${attrKeyword}"]`,
                    `[class*="${attrKeyword}"]`,
                    `[aria-label*="${attrKeyword}"]`
                ];
                for (const selector of selectors) {
                    try {
                        const containers = searchContainer.querySelectorAll(selector);
                        for (const container of containers) {
                            const buttons = container.querySelectorAll('button, a, [role="button"]');
                            for (const button of buttons) {
                                const btnText = (button.innerText?.toLowerCase().trim() || '') + ' ' +
                                               (button.getAttribute('aria-label')?.toLowerCase() || '');

                                // Skip reject buttons
                                if (rejectKeywords.some(kw => btnText.includes(kw))) continue;

                                // Click accept buttons
                                if (acceptKeywords.some(kw => btnText.includes(kw))) {
                                    if (tryClick(button, 'container search')) return true;
                                }
                            }
                        }
                    } catch (e) {
                        // Ignore selector errors
                    }
                }
            }
        }

        return false;
    }

    let attempts = 0;
    let pollingInterval = null;
    let observer = null;

    // IMPROVEMENT 3: Active polling for first 10 seconds (handles slow-loading modals like OneTrust)
    function startActivePolling() {
        console.log('[hls-generator] Starting active polling (500ms intervals for 10 seconds)...');

        pollingInterval = setInterval(() => {
            attempts++;

            if (findAndClickCookieConsent()) {
                console.log('[hls-generator] Cookie consent handled via polling (attempt ' + attempts + ')');
                clearInterval(pollingInterval);
                if (observer) observer.disconnect();
                return;
            }

            // After 20 attempts (10 seconds), switch to passive observer
            if (attempts >= 20) {
                console.log('[hls-generator] Polling complete. Switching to passive MutationObserver...');
                clearInterval(pollingInterval);
                startPassiveObserver();
            }
        }, 500);
    }

    // Passive MutationObserver for remaining time
    function startPassiveObserver() {
        observer = new MutationObserver((mutations, obs) => {
            if (findAndClickCookieConsent()) {
                console.log('[hls-generator] Cookie consent handled via MutationObserver');
                obs.disconnect();
                if (pollingInterval) clearInterval(pollingInterval);
            }
        });

        observer.observe(document.body, {
            childList: true,
            subtree: true
        });

        console.log('[hls-generator] MutationObserver active...');

        // Total timeout: 30 seconds (10s polling + 20s observer)
        setTimeout(() => {
            if (observer) observer.disconnect();
            console.log('[hls-generator] Cookie consent detection timeout (30s)');
        }, 20000);
    }

    // Try immediately on script load
    if (findAndClickCookieConsent()) {
        console.log('[hls-generator] Cookie consent handled immediately');
    } else {
        // Start active polling
        startActivePolling();
    }
})();
