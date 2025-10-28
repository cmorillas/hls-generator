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

    // --- Constantes Globales (movidas al scope principal para ser accesibles por todas las funciones) ---
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

    const rejectKeywords = [
        'reject', 'decline', 'deny', 'refuse', 'customize', 'settings',
        'manage', 'preferences', 'only necessary', 'only essential',
        'rechazar', 'denegar', 'personalizar', 'configurar',
        'ablehnen', 'anpassen', 'rifiuta', 'recusar'
    ];

    const selectorPatterns = ['button', 'a', '[role="button"]', 'div[onclick]'];
    // --- Fin de Constantes Globales ---

    function findAndClickCookieConsent() {
        // BUG FIX: La función tryClick estaba duplicada. Se ha eliminado esta versión
        // y se utiliza la que está en el scope principal.

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

    // --- Estrategia Específica para YouTube (más rápida y directa) ---
    function findAndClickYouTubeConsent() {
        const youtubeSpecificSelectors = [
            'button.ytp-consent-button-modern', // Consentimiento en el reproductor
            'ytd-consent-bump-v2-lightbox tp-yt-paper-button', // Diálogo principal
            'tp-yt-paper-button[aria-label*="Accept"]',
            'tp-yt-paper-button[aria-label*="Aceptar"]',
            'tp-yt-paper-button[aria-label*="Akzeptieren"]',
        ];

        // BUG FIX: rejectKeywords ahora es accesible desde este scope.
        for (const selector of youtubeSpecificSelectors) {
            const buttons = document.querySelectorAll(selector);
            for (const el of buttons) {
                const elementText = (el.innerText?.toLowerCase().trim() || '') + ' ' +
                                    (el.getAttribute('aria-label')?.toLowerCase() || '');
                if (!rejectKeywords.some(keyword => elementText.includes(keyword))) {
                    if (tryClick(el, 'YouTube-specific selector: ' + selector)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    const tryClick = (element, reason) => {
        if (element && typeof element.click === 'function') {
            if (element.offsetParent !== null || element.checkVisibility?.() !== false) {
                const text = element.innerText?.trim() || element.getAttribute('aria-label') || '';
                console.log('[hls-generator] ✓ Clicking consent button:', text, '(Reason:', reason, ')');
                element.click();
                return true;
            }
        }
        return false;
    };

    // IMPROVEMENT 3: Active polling for first 10 seconds (handles slow-loading modals like OneTrust)
    function startActivePolling() {
        console.log('[hls-generator] Starting active polling (500ms intervals for 10 seconds)...');

        pollingInterval = setInterval(() => {
            attempts++;

            // En YouTube, solo usamos la función específica y rápida.
            const clicked = window.location.hostname.includes('youtube.com')
                ? findAndClickYouTubeConsent()
                : findAndClickCookieConsent();

            if (clicked) {
                console.log('[hls-generator] Cookie consent handled via polling (attempt ' + attempts + ')');
                clearInterval(pollingInterval);
                if (observer) observer.disconnect();
                return;
            }

            // After 20 attempts (10 seconds), switch to passive observer
            // O si es YouTube y falla, pasamos directamente al observador genérico como fallback.
            if (attempts >= 20 || (window.location.hostname.includes('youtube.com') && attempts >= 10)) {
                if (window.location.hostname.includes('youtube.com')) {
                    console.log('[hls-generator] YouTube-specific polling failed. Switching to generic observer as fallback.');
                }
                console.log('[hls-generator] Polling complete. Switching to passive MutationObserver...');
                clearInterval(pollingInterval);
                startPassiveObserver();
            }
        }, 500);
    }

    // Passive MutationObserver for remaining time
    function startPassiveObserver() {
        // BUG FIX: El observer ahora es consciente del contexto (YouTube vs Genérico)
        // para no usar la búsqueda lenta en YouTube.
        observer = new MutationObserver((mutations, obs) => {
            const clicked = window.location.hostname.includes('youtube.com')
                ? findAndClickYouTubeConsent()
                : findAndClickCookieConsent();

            if (clicked) {
                console.log('[hls-generator] Cookie consent handled via MutationObserver');
                obs.disconnect();
                if (pollingInterval) {
                    clearInterval(pollingInterval);
                }
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
    if (window.location.hostname.includes('youtube.com')) {
        console.log('[hls-generator] YouTube detected. Using fast-path polling.');
        if (findAndClickYouTubeConsent()) {
            console.log('[hls-generator] Cookie consent handled immediately on YouTube');
        } else {
            startActivePolling();
        }
    } else {
        console.log('[hls-generator] Generic site detected. Using standard polling.');
        if (findAndClickCookieConsent()) {
            console.log('[hls-generator] Cookie consent handled immediately');
        } else {
            startActivePolling();
        }
    }
})();
