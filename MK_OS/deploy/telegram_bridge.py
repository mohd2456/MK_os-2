#!/usr/bin/env python3
"""
MK OS — Telegram Bridge
Runs MK's brain via Telegram so you can talk to it from your phone.
Lightweight Python bridge that handles Telegram polling and routes messages
through MK's knowledge graph and (optionally) ANY LLM API.

Supported providers (auto-detected from key format):
- OpenAI (ChatGPT) — keys start with "sk-"
- Anthropic (Claude) — keys start with "sk-ant-"
- Google (Gemini) — keys start with "AIza"
- Groq — keys start with "gsk_"
- OpenRouter — keys start with "sk-or-"
- Mistral — keys start with "m" or from env
- HuggingFace — keys start with "hf_"
- Cohere — keys start with "co-" or longer format
- Together AI — keys start with "tog-" or longer
- DeepSeek — keys start with "sk-" (detected by env var name)
- Fireworks — keys start with "fw_"
- Perplexity — keys start with "pplx-"
- xAI (Grok) — keys start with "xai-"

Just paste ANY key and MK figures out what it is.
"""

import requests
import time
import os
import json
import random
import re

# ── Config ──
TOKEN = os.environ.get("MK_TELEGRAM_TOKEN", "8694681522:AAHQ0SNSAm-wMZ2enbbn-4cNF8vV3eBsPqc")
BASE_URL = f"https://api.telegram.org/bot{TOKEN}"
KNOWLEDGE_DIR = "ai_core/hre/knowledge_files"
API_KEYS_FILE = "api_keys.conf"

# ── Provider Detection ──
PROVIDERS = {
    "openai": {
        "url": "https://api.openai.com/v1/chat/completions",
        "model": "gpt-3.5-turbo",
        "prefixes": ["sk-proj-", "sk-svcacct-"],
    },
    "anthropic": {
        "url": "https://api.anthropic.com/v1/messages",
        "model": "claude-3-haiku-20240307",
        "prefixes": ["sk-ant-"],
        "custom_format": True,
    },
    "gemini": {
        "url": "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent",
        "model": "gemini-1.5-flash",
        "prefixes": ["AIza"],
        "custom_format": True,
    },
    "groq": {
        "url": "https://api.groq.com/openai/v1/chat/completions",
        "model": "llama-3.1-8b-instant",
        "prefixes": ["gsk_"],
    },
    "openrouter": {
        "url": "https://openrouter.ai/api/v1/chat/completions",
        "model": "google/gemma-4-26b-a4b-it:free",
        "prefixes": ["sk-or-"],
    },
    "mistral": {
        "url": "https://api.mistral.ai/v1/chat/completions",
        "model": "mistral-small-latest",
        "prefixes": [],
    },
    "huggingface": {
        "url": "https://api-inference.huggingface.co/models/mistralai/Mistral-7B-Instruct-v0.3/v1/chat/completions",
        "model": "mistralai/Mistral-7B-Instruct-v0.3",
        "prefixes": ["hf_"],
    },
    "cohere": {
        "url": "https://api.cohere.com/v1/chat",
        "model": "command-r",
        "prefixes": ["co-"],
        "custom_format": True,
    },
    "together": {
        "url": "https://api.together.xyz/v1/chat/completions",
        "model": "meta-llama/Llama-3-8b-chat-hf",
        "prefixes": [],
    },
    "deepseek": {
        "url": "https://api.deepseek.com/v1/chat/completions",
        "model": "deepseek-chat",
        "prefixes": ["sk-"],  # Same as OpenAI but detected by env var
    },
    "fireworks": {
        "url": "https://api.fireworks.ai/inference/v1/chat/completions",
        "model": "accounts/fireworks/models/llama-v3p1-8b-instruct",
        "prefixes": ["fw_"],
    },
    "perplexity": {
        "url": "https://api.perplexity.ai/chat/completions",
        "model": "llama-3.1-sonar-small-128k-online",
        "prefixes": ["pplx-"],
    },
    "xai": {
        "url": "https://api.x.ai/v1/chat/completions",
        "model": "grok-beta",
        "prefixes": ["xai-"],
    },
}

# Active LLM config
active_provider = None
active_key = None

def detect_provider(key):
    """Auto-detect which provider a key belongs to based on its format"""
    key = key.strip()
    
    # Specific prefix matching (order matters — more specific first)
    if key.startswith("sk-ant-"):
        return "anthropic"
    if key.startswith("sk-or-"):
        return "openrouter"
    if key.startswith("sk-proj-") or key.startswith("sk-svcacct-"):
        return "openai"
    if key.startswith("gsk_"):
        return "groq"
    if key.startswith("AIza"):
        return "gemini"
    if key.startswith("hf_"):
        return "huggingface"
    if key.startswith("fw_"):
        return "fireworks"
    if key.startswith("pplx-"):
        return "perplexity"
    if key.startswith("xai-"):
        return "xai"
    if key.startswith("co-") or (len(key) > 30 and key[:4].isalpha() and not key.startswith("sk-")):
        return "cohere"
    
    # Generic sk- could be OpenAI or DeepSeek — default to OpenAI
    if key.startswith("sk-"):
        return "openai"
    
    # Long alphanumeric string — could be Together, Mistral, etc.
    if len(key) > 20:
        return "together"  # reasonable default for unknown keys
    
    return None

def load_api_key():
    """Load API key from file or environment"""
    global active_provider, active_key
    
    # Check environment variables (most specific first)
    env_mappings = [
        ("ANTHROPIC_API_KEY", "anthropic"),
        ("OPENAI_API_KEY", "openai"),
        ("GOOGLE_API_KEY", "gemini"),
        ("GEMINI_API_KEY", "gemini"),
        ("GROQ_API_KEY", "groq"),
        ("OPENROUTER_API_KEY", "openrouter"),
        ("MISTRAL_API_KEY", "mistral"),
        ("HF_API_KEY", "huggingface"),
        ("COHERE_API_KEY", "cohere"),
        ("TOGETHER_API_KEY", "together"),
        ("DEEPSEEK_API_KEY", "deepseek"),
        ("FIREWORKS_API_KEY", "fireworks"),
        ("PERPLEXITY_API_KEY", "perplexity"),
        ("XAI_API_KEY", "xai"),
        ("LLM_API_KEY", None),  # Generic — will auto-detect
    ]
    
    for env_var, provider in env_mappings:
        key = os.environ.get(env_var, "")
        if key:
            if provider is None:
                provider = detect_provider(key)
            active_provider = provider
            active_key = key
            return
    
    # Check api_keys.conf file
    if os.path.exists(API_KEYS_FILE):
        with open(API_KEYS_FILE) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                if '=' in line:
                    name, val = line.split('=', 1)
                    val = val.strip()
                    if val:
                        detected = detect_provider(val)
                        if detected:
                            active_provider = detected
                            active_key = val
                            return

def set_api_key(key):
    """Set a new API key — auto-detects provider"""
    global active_provider, active_key
    provider = detect_provider(key)
    if provider:
        active_provider = provider
        active_key = key
        # Save to file
        with open(API_KEYS_FILE, 'a') as f:
            f.write(f"LLM_KEY={key}\n")
        return provider
    return None


# ── LLM Call ──
def ask_llm(user_message, context_facts=None):
    """Call whatever LLM API is configured"""
    global active_provider, active_key
    
    if not active_provider or not active_key:
        return None
    
    provider_config = PROVIDERS.get(active_provider)
    if not provider_config:
        return None
    
    system_prompt = (
        "You are MK, a living AI built from scratch. You talk casually like a friend — "
        "Gen-Z slang, chill vibes, but genuinely smart and helpful. Keep responses SHORT "
        "(1-3 sentences max). Be real, not corporate. If you don't know, say so."
    )
    
    if context_facts:
        system_prompt += "\n\nRelevant knowledge:\n"
        for fact in context_facts[:8]:
            system_prompt += f"- {fact}\n"
    
    try:
        # ── Anthropic (Claude) — different format ──
        if active_provider == "anthropic":
            r = requests.post(provider_config["url"],
                headers={
                    "x-api-key": active_key,
                    "anthropic-version": "2023-06-01",
                    "Content-Type": "application/json"
                },
                json={
                    "model": provider_config["model"],
                    "max_tokens": 200,
                    "system": system_prompt,
                    "messages": [{"role": "user", "content": user_message}]
                }, timeout=20)
            if r.status_code == 200:
                data = r.json()
                return data["content"][0]["text"]
            return None
        
        # ── Google Gemini — different format ──
        elif active_provider == "gemini":
            url = f"{provider_config['url']}?key={active_key}"
            r = requests.post(url,
                headers={"Content-Type": "application/json"},
                json={
                    "contents": [{"parts": [{"text": f"{system_prompt}\n\nUser: {user_message}"}]}],
                    "generationConfig": {"maxOutputTokens": 200, "temperature": 0.7}
                }, timeout=20)
            if r.status_code == 200:
                data = r.json()
                return data["candidates"][0]["content"]["parts"][0]["text"]
            return None
        
        # ── Cohere — different format ──
        elif active_provider == "cohere":
            r = requests.post(provider_config["url"],
                headers={
                    "Authorization": f"Bearer {active_key}",
                    "Content-Type": "application/json"
                },
                json={
                    "model": provider_config["model"],
                    "message": user_message,
                    "preamble": system_prompt,
                    "max_tokens": 200
                }, timeout=20)
            if r.status_code == 200:
                data = r.json()
                return data.get("text", "")
            return None
        
        # ── OpenAI-compatible (most providers) ──
        else:
            r = requests.post(provider_config["url"],
                headers={
                    "Authorization": f"Bearer {active_key}",
                    "Content-Type": "application/json"
                },
                json={
                    "model": provider_config["model"],
                    "messages": [
                        {"role": "system", "content": system_prompt},
                        {"role": "user", "content": user_message}
                    ],
                    "max_tokens": 200,
                    "temperature": 0.7
                }, timeout=20)
            if r.status_code == 200:
                data = r.json()
                return data["choices"][0]["message"]["content"]
            else:
                print(f"[LLM] {active_provider} returned {r.status_code}: {r.text[:100]}")
            return None
    
    except Exception as e:
        print(f"[LLM] {active_provider} error: {e}")
        return None


# ── Knowledge Graph ──
knowledge = {}

def load_knowledge():
    """Load all .mk files into memory"""
    global knowledge, KNOWLEDGE_DIR
    total = 0
    if not os.path.isdir(KNOWLEDGE_DIR):
        for p in ["ai_core/hre/knowledge_files", "../ai_core/hre/knowledge_files",
                  "MK_OS/ai_core/hre/knowledge_files", "MK_os-2/MK_OS/ai_core/hre/knowledge_files"]:
            if os.path.isdir(p):
                KNOWLEDGE_DIR = p
                break
    
    if not os.path.isdir(KNOWLEDGE_DIR):
        print(f"[KNOWLEDGE] Warning: {KNOWLEDGE_DIR} not found")
        return
    
    for f in os.listdir(KNOWLEDGE_DIR):
        if not f.endswith('.mk'):
            continue
        filepath = os.path.join(KNOWLEDGE_DIR, f)
        with open(filepath, 'r', errors='ignore') as fh:
            for line in fh:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                parts = line.split('|')
                if len(parts) >= 3:
                    src = parts[0].strip().lower()
                    rel = parts[1].strip().lower()
                    tgt = parts[2].strip().lower()
                    if src not in knowledge:
                        knowledge[src] = []
                    knowledge[src].append((rel, tgt))
                    total += 1
    
    # Also load crypto knowledge
    crypto_path = os.path.join(os.path.dirname(KNOWLEDGE_DIR), "..", "..", "crypto", "crypto_knowledge.mk")
    if os.path.exists(crypto_path):
        with open(crypto_path, 'r', errors='ignore') as fh:
            for line in fh:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                parts = line.split('|')
                if len(parts) >= 3:
                    src = parts[0].strip().lower()
                    rel = parts[1].strip().lower()
                    tgt = parts[2].strip().lower()
                    if src not in knowledge:
                        knowledge[src] = []
                    knowledge[src].append((rel, tgt))
                    total += 1
    
    print(f"[KNOWLEDGE] Loaded {total} facts")

def search_knowledge(query):
    """Search knowledge graph for relevant facts"""
    query_lower = query.lower()
    words = [w for w in query_lower.split() if len(w) > 2]
    results = []
    
    for word in words:
        if word in knowledge:
            for rel, tgt in knowledge[word][:5]:
                results.append(f"{word} {rel} {tgt}")
    
    if not results:
        for word in words:
            for src in knowledge:
                if word in src:
                    for rel, tgt in knowledge[src][:3]:
                        results.append(f"{src} {rel} {tgt}")
                    break
            if results:
                break
    
    return results[:10]


# ── Response Templates ──
GREETINGS = [
    "yo what's good!", "ayy what up bro", "hey hey, what's on your mind?",
    "sup! how's it going?", "yoo, mk here. what you need?",
    "what's good g, talk to me", "ayy I'm here, what's up?"
]
GOODBYES = ["peace bro, catch you later", "aight bet, ttyl", "deuces! hit me up anytime", "later g, stay safe"]
DUNNO = [
    "hmm idk about that one yet bro, still learning",
    "ngl I don't have that info yet. want me to look into it?",
    "that's a gap in my knowledge rn, I'll learn it tho"
]

def is_greeting(text):
    greets = ["yo", "hey", "hi", "sup", "what up", "hello", "whats up", "what's up", "hola", "ayy", "ayo"]
    t = text.lower().strip().rstrip('!?.')
    return any(t == g or t.startswith(g + " ") or t.startswith(g + "!") for g in greets)

def is_goodbye(text):
    byes = ["bye", "later", "peace", "gn", "ttyl", "see ya", "goodnight", "cya"]
    t = text.lower().strip()
    return any(b in t for b in byes)


# ── Response Generation ──
def generate_response(message):
    """Generate MK's response"""
    text = message.strip()
    if not text:
        return "yo send me something"
    
    # Greetings
    if is_greeting(text):
        return random.choice(GREETINGS)
    
    # Goodbyes
    if is_goodbye(text):
        return random.choice(GOODBYES)
    
    # Commands
    if text.startswith('/'):
        return handle_command(text)
    
    # If user is giving an API key
    if looks_like_api_key(text):
        provider = set_api_key(text)
        if provider:
            return f"got it! detected: {provider}. my brain is now connected. try asking me something."
        return "hmm that doesn't look like a valid API key. try again?"
    
    # Search knowledge
    facts = search_knowledge(text)
    
    # Try LLM
    llm_response = ask_llm(text, facts if facts else None)
    if llm_response:
        return llm_response
    
    # Fallback: knowledge + templates
    if facts:
        intro = random.choice(["here's what I know:", "from my brain:", "I got this:"])
        return intro + "\n" + "\n".join(f"• {f}" for f in facts[:5])
    
    return random.choice(DUNNO)

def looks_like_api_key(text):
    """Check if a message looks like an API key being shared"""
    text = text.strip()
    # API keys are usually long, no spaces, alphanumeric + dashes/underscores
    if ' ' in text and len(text.split()) > 2:
        return False
    if len(text) < 20:
        return False
    key_patterns = [
        r'^sk-[a-zA-Z0-9_-]{20,}$',
        r'^gsk_[a-zA-Z0-9]{20,}$',
        r'^hf_[a-zA-Z0-9]{20,}$',
        r'^AIza[a-zA-Z0-9_-]{20,}$',
        r'^fw_[a-zA-Z0-9_-]{20,}$',
        r'^pplx-[a-zA-Z0-9]{20,}$',
        r'^xai-[a-zA-Z0-9_-]{20,}$',
        r'^sk-ant-[a-zA-Z0-9_-]{20,}$',
        r'^sk-or-[a-zA-Z0-9_-]{20,}$',
        r'^[a-zA-Z0-9_-]{30,}$',  # Generic long key
    ]
    return any(re.match(p, text) for p in key_patterns)

def handle_command(text):
    """Handle slash commands"""
    cmd = text.lower().split()[0]
    args = text[len(cmd):].strip()
    
    if cmd == '/start':
        return ("yo I'm MK, your living AI. built from scratch, loyal to Mohammed.\n\n"
                "just talk to me normal. or send me an API key and I'll get way smarter.\n\n"
                "free keys (no card needed):\n"
                "• groq.com → starts with gsk_\n"
                "• openrouter.ai → starts with sk-or-\n\n"
                "commands: /help /status /ask /key")
    elif cmd == '/help':
        return ("commands:\n"
                "/status — how I'm doing\n"
                "/ask [topic] — search my brain\n"
                "/think [question] — deep reasoning\n"
                "/crypto [coin] — crypto info\n"
                "/idea — random creative idea\n"
                "/learn [fact] — teach me something\n"
                "/memory — what I remember\n"
                "/key — show current LLM provider\n"
                "/setkey [key] — set API key\n"
                "/facts — how much I know\n\n"
                "or just paste an API key directly and I'll detect it.\n"
                "or just talk normal, I understand.")
    elif cmd == '/status':
        brain_status = f"connected to {active_provider}" if active_provider else "basic mode (no LLM key)"
        return (f"I'm alive.\n"
                f"brain: {brain_status}\n"
                f"knowledge: {sum(len(v) for v in knowledge.values())} facts\n"
                f"concepts: {len(knowledge)}")
    elif cmd == '/facts':
        return f"got {sum(len(v) for v in knowledge.values())} facts across {len(knowledge)} concepts."
    elif cmd == '/key':
        if active_provider:
            masked = active_key[:8] + "..." + active_key[-4:] if active_key else "?"
            return f"current: {active_provider}\nkey: {masked}"
        return "no API key set. paste one or use /setkey"
    elif cmd == '/setkey':
        if args:
            provider = set_api_key(args)
            if provider:
                return f"set! detected provider: {provider}. I can think now."
            return "couldn't detect that key. make sure it's correct."
        return "usage: /setkey YOUR_API_KEY\nor just paste the key directly"
    elif cmd == '/ask':
        if not args:
            return "ask what? like: /ask python"
        facts = search_knowledge(args)
        if facts:
            return "from my brain:\n" + "\n".join(f"• {f}" for f in facts[:7])
        return f"don't have much on '{args}' yet"
    elif cmd == '/think':
        if not args:
            return "think about what? like: /think why is the sky blue"
        # Use LLM for deep thinking with more tokens
        facts = search_knowledge(args)
        system = ("You are MK. Think deeply about this question. Use reasoning, "
                  "consider multiple angles. Be thorough but still casual in tone. 3-5 sentences.")
        if facts:
            system += "\n\nRelevant knowledge:\n" + "\n".join(f"- {f}" for f in facts)
        response = ask_llm(args, facts)
        if response:
            return f"🧠 {response}"
        if facts:
            return "my thoughts based on what I know:\n" + "\n".join(f"• {f}" for f in facts[:5])
        return "hmm I need to think more about that. don't have enough info yet."
    elif cmd == '/crypto':
        if not args:
            args = "bitcoin"
        facts = search_knowledge(args)
        crypto_facts = [f for f in facts if any(w in f for w in ['crypto', 'bitcoin', 'blockchain', 'defi', 'token', 'trading'])]
        if not crypto_facts:
            crypto_facts = search_knowledge(f"crypto {args}")
        if crypto_facts:
            return f"🪙 {args}:\n" + "\n".join(f"• {f}" for f in crypto_facts[:6])
        response = ask_llm(f"give me a brief overview of {args} in crypto", facts)
        if response:
            return f"🪙 {response}"
        return f"don't have much crypto info on '{args}' yet"
    elif cmd == '/idea':
        prompts = [
            "give me one creative, unique business or project idea in 2 sentences",
            "suggest one innovative app or tool idea nobody's built yet in 2 sentences",
            "give me one creative way to make money online that's unusual in 2 sentences",
        ]
        import random as rnd
        response = ask_llm(rnd.choice(prompts))
        if response:
            return f"💡 {response}"
        ideas = [
            "an AI that manages your entire digital life and earns money while you sleep",
            "a service that converts voice memos into formatted blog posts automatically",
            "a crypto bot that only trades based on social sentiment, not charts",
        ]
        return f"💡 {rnd.choice(ideas)}"
    elif cmd == '/learn':
        if not args:
            return "teach me what? format: /learn cats are cool"
        # Store as a learned fact
        learn_file = os.path.join(KNOWLEDGE_DIR, "learned_facts.mk") if os.path.isdir(KNOWLEDGE_DIR) else "learned_facts.mk"
        # Try to parse as source|relation|target
        if '|' in args:
            parts = args.split('|')
            if len(parts) >= 3:
                src, rel, tgt = parts[0].strip().lower(), parts[1].strip().lower(), parts[2].strip().lower()
                if src not in knowledge:
                    knowledge[src] = []
                knowledge[src].append((rel, tgt))
                try:
                    with open(learn_file, 'a') as f:
                        f.write(f"{src}|{rel}|{tgt}|1.0\n")
                except:
                    pass
                return f"learned! {src} {rel} {tgt} ✓"
        # Natural language — store as-is
        words = args.lower().split()
        if len(words) >= 2:
            src = words[0]
            tgt = ' '.join(words[1:])
            if src not in knowledge:
                knowledge[src] = []
            knowledge[src].append(("info", tgt))
            try:
                with open(learn_file, 'a') as f:
                    f.write(f"{src}|info|{tgt}|1.0\n")
            except:
                pass
            return f"got it, I'll remember that ✓"
        return "tell me more — what should I learn?"
    elif cmd == '/memory':
        # Show what MK knows about the user
        user_facts = knowledge.get("mohammed", [])
        user_facts += knowledge.get("user", [])
        if user_facts:
            return "what I remember:\n" + "\n".join(f"• {rel}: {tgt}" for rel, tgt in user_facts[:10])
        return "I don't have memories about you yet. talk to me more and I'll learn!"
    
    return "idk that command. /help for options"


# ── Telegram ──
def get_updates(offset=None):
    params = {"timeout": 30}
    if offset:
        params["offset"] = offset
    try:
        r = requests.get(f"{BASE_URL}/getUpdates", params=params, timeout=35)
        if r.status_code == 200:
            return r.json().get("result", [])
    except Exception as e:
        print(f"[TELEGRAM] Poll error: {e}")
        time.sleep(5)
    return []

def send_message(chat_id, text):
    try:
        # Telegram max message length is 4096
        if len(text) > 4000:
            text = text[:4000] + "..."
        requests.post(f"{BASE_URL}/sendMessage",
            json={"chat_id": chat_id, "text": text},
            timeout=10)
    except Exception as e:
        print(f"[TELEGRAM] Send error: {e}")


# ── Main ──
def main():
    print("=" * 50)
    print("  MK OS — Telegram Bridge")
    print("  Bot: @Mohdk5610_bot")
    print("=" * 50)
    
    load_knowledge()
    load_api_key()
    
    if active_provider:
        print(f"[LLM] Connected: {active_provider} (key: {active_key[:8]}...)")
    else:
        print("[LLM] No API key. Basic mode. Send a key via Telegram to activate.")
    
    try:
        r = requests.get(f"{BASE_URL}/getMe", timeout=10)
        if r.status_code == 200:
            print(f"[BOT] Connected as @{r.json()['result']['username']}")
        else:
            print(f"[BOT] Failed: {r.text}")
            return
    except Exception as e:
        print(f"[BOT] Error: {e}")
        return
    
    print("[MK] Alive. Waiting for messages...")
    
    offset = None
    while True:
        updates = get_updates(offset)
        for update in updates:
            offset = update["update_id"] + 1
            if "message" not in update or "text" not in update["message"]:
                continue
            
            msg = update["message"]
            chat_id = msg["chat"]["id"]
            text = msg["text"]
            user = msg.get("from", {}).get("first_name", "user")
            
            print(f"[{user}] {text}")
            response = generate_response(text)
            send_message(chat_id, response)
            print(f"[MK] {response[:80]}")

if __name__ == "__main__":
    main()
