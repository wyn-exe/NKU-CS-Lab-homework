// script.js
const container = document.getElementById('emoji-container');
const emojis = ['😊', '😂', '😍', '🥺', '😎', '😜', '🤣', '💀', '🔥', '🌈', '✨', '🌟','🥰','😅','😗','🤔','😶‍🌫️','😴','🥵','🤯','🥳','🥺','👻','🤬','😽','💖','💯','💢','😎','😇']; // Emoji 可自定义

// 控制生成新 Emoji 的时间间隔
const generateInterval = 300; // 每100ms生成一个新的emoji

function createEmoji() {
    const emoji = document.createElement('div');
    emoji.classList.add('emoji');
    
    // 随机选择一个emoji
    emoji.innerText = emojis[Math.floor(Math.random() * emojis.length)];
    
    // 随机生成初始位置（水平位置）
    const startX = Math.random() * window.innerWidth;
    emoji.style.left = `${startX}px`;

    // 随机设置不同的动画时长（控制下落速度）
    const fallDuration = Math.random() * 3 + 4; // 在4到7秒之间随机
    
    // 应用动画时长
    emoji.style.animationDuration = `${fallDuration}s`;

    container.appendChild(emoji);

    // 动画结束后删除emoji
    emoji.addEventListener('animationend', () => {
        emoji.remove();
    });
}

// 持续生成新的 Emoji
setInterval(createEmoji, generateInterval);

