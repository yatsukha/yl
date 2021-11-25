'use strict';

window.setTimeout(() => {
//


  function assert(condition, message) {
    if (!condition) {
      throw new Error(message || "Assertion failed.");
    }
  }

  class Terminal {
    constructor(prompt = '', properties = {canvasId: 'canvas'}) {
      assert(properties.canvasId != undefined);
      
      this.canvas = document.getElementById(properties.canvasId);
      assert(this.canvas);
      
      this.prompt = prompt;
      
      this.canvas.width = this.canvas.getBoundingClientRect().width;
      this.canvas.height = this.canvas.getBoundingClientRect().height;
      
      this.ctx = this.canvas.getContext('2d');
      this.ctx.font = properties.font || '14px monospace';
      
      // metrics
      
      const text = 'g{^Mwy';
      const metrics = this.ctx.measureText(text);
      
      this.textMetrics = {
        charWidth: metrics.width / text.length,
        charHeight: metrics.actualBoundingBoxAscent,
        lineSpacing: metrics.actualBoundingBoxDescent + 1
      };
      
      this.position = {
        col: this.prompt.length,
        y: this.textMetrics.charHeight
      };
      
      this.cursor = {
        width: this.textMetrics.charWidth,
        negativeHeight: -this.textMetrics.charHeight,
        positiveHeight: this.textMetrics.lineSpacing,
        height: this.textMetrics.charHeight + this.textMetrics.lineSpacing
      };
      
      // buffer to act as text storage
      this.buffer = prompt;
      this.ctx.fillText(this.buffer, 0, this.position.y);
      this.fillCursor();
    }

    cls() {
      this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
      this.position.y = this.textMetrics.charHeight;
      this.position.col = this.prompt.length;

      this.buffer = prompt;
      this.ctx.fillText(this.buffer, 0, this.position.y);
      this.fillCursor();
    }
    
    setBlack() {
      this.ctx.fillStyle = 'black';
    }
    
    setWhite() {
      this.ctx.fillStyle = 'white';
    }
    
    fillCursor() {
      this.setBlack();
      this.ctx.fillRect(
        this.position.col * this.textMetrics.charWidth, this.position.y + this.cursor.negativeHeight, 
        this.cursor.width, this.cursor.height
      );
      if (this.position.col != this.buffer.length) {
        this.setWhite();
        this.ctx.fillText(
          this.buffer[this.position.col], 
          this.position.col * this.textMetrics.charWidth, 
          this.position.y);
        this.setBlack();
      }
    }
    
    clearCursor() {
      this.ctx.clearRect(
        this.position.col * this.textMetrics.charWidth - 0.75, this.position.y + this.cursor.negativeHeight, 
        this.cursor.width + 2, this.cursor.height * 2
      );
      if (this.position.col < this.buffer.length) {
        this.setBlack();
        this.ctx.fillText(
          this.buffer[this.position.col], 
          this.position.col * this.textMetrics.charWidth, 
          this.position.y);
      }
    }
    
    append(str) {
      this.clearCursor();
      
      this.buffer = 
        this.buffer.slice(0, this.position.col) +
        str +
        this.buffer.slice(this.position.col);
      
      this.setBlack();
      this.clear(this.position.col);
      this.ctx.fillText(this.buffer.slice(this.position.col), this.position.col * this.textMetrics.charWidth, this.position.y);
      this.position.col += str.length;
      this.fillCursor();
      this.focus();
    }
    
    pop() {
      if (this.position.col <= prompt.length) {
        return;
      }

      const ret = this.buffer[this.position.col - 1];
      
      this.clear(this.position.col - 1);
      this.buffer = 
        this.buffer.slice(0, this.position.col - 1) +
        this.buffer.slice(this.position.col);
      --this.position.col;
      this.append('');
      this.fillCursor();

      return ret;
    }
    
    clear(offset = undefined) {
      if (!offset) this.clearCursor();
      this.ctx.clearRect(
        (offset || this.prompt.length) * this.textMetrics.charWidth - 0.75, 
        this.position.y + this.cursor.negativeHeight, 			
        this.canvas.width, this.cursor.height * 2);
      if (!offset) {
        this.buffer = this.prompt;
        this.position.col = prompt.length;
        this.fillCursor();
      }
    }

    focus() {
      const doc = document.documentElement;
      const vh = 
        Math.max(document.documentElement.clientHeight || 0, window.innerHeight || 0);
     
      var pos = (window.pageYOffset || doc.scrollTop)  - (doc.clientTop || 0);
      if (this.position.y + this.cursor.height > pos + vh) {
        window.scroll(0, Math.max(0, this.position.y - vh + this.cursor.height));
      } else if (this.position.y < pos) {
        window.scroll(0, this.position.y - this.cursor.height);
      }
    }
    
    newLine(echoed = false) {
      if (!echoed) {
        this.clearCursor();
        if (this.position.col < this.buffer.length) {
          this.ctx.fillText(
            this.buffer[this.position.col], 
            this.position.col * this.textMetrics.charWidth, this.position.y);
        }
      }
      
      this.buffer = this.prompt;
      this.position.col = this.prompt.length;
      this.position.y += this.cursor.height;
      this.ctx.fillText(this.prompt, 0, this.position.y);
      this.append('');
      this.fillCursor();
      
      this.focus();
    }
    
    jump(offset) {
      const tmp = Math.max(this.prompt.length, Math.min(this.buffer.length, this.position.col + offset));
      this.clearCursor();
      this.position.col = tmp;
      this.fillCursor();
      this.focus();
    }

    breakLine() {
      const ret  = this.buffer.slice(this.prompt.length, this.position.col);
      var indent = 0;
      while (indent < ret.length && ret[indent] == ' ') {
        ++indent;
      }
      const cont = ' '.repeat(indent) + this.buffer.slice(this.position.col);
      this.clear(this.position.col);
      this.newLine(true);
      this.append(cont);
      this.jump(-1000000);
      this.jump(indent);
      return ret;
    }
    
    echo(str) {
      this.clearCursor();
      this.ctx.clearRect(
        0, this.position.y + this.cursor.negativeHeight, 			
        this.canvas.width, this.cursor.height * 2);
      this.setBlack();
      this.ctx.fillText(str, 0, this.position.y);
      this.newLine(true);
    }
    
    get line() {
      return this.buffer.slice(this.prompt.length);
    }
  };

  const prompt = 'yl> ';
  const cont_prompt = '... ';
  const term = new Terminal(prompt);
  var storage = "";
  var parens = 0;
  var historyIdx = -1;
  const matching = {
    '(': ')',
    '{': '}',
    '[': ']',
    '"': '"'
  };

  term.echo("yatsukha's lisp");
  term.echo("use 'help' to get started, note that some features are currently missing from the web version");
  term.echo("press shift + enter to break an expression into multiple lines");
  term.echo("use 'cls' to clear the screen");
  term.echo(Module.load_predef(".predef.yl").length ? "failed to load predef" : "loaded predef");
  term.echo("");

  function preventDefault(event) {
    event.preventDefault();
    event.stopPropagation();
  }

  document.addEventListener("keydown", event => {
    const text = event.key;
    if (!(event.ctrlKey || event.metaKey) && text.length == 1 && !event.location) {
      term.append(text);
      switch (event.which) {
        case 32:
          preventDefault(event);
          break;
        default:
          if (matching[text]) {
            term.append(matching[text]);
            term.jump(-1);
          }
          break;
      }
    } else {
      switch (event.which) {
        case 8:
          const c = term.pop();
          if (matching[c] && matching[c] == term.buffer[term.position.col]) {
            term.jump(1);
            term.pop();
          }
          preventDefault(event);
          break;
        case 9:
          term.append('  ');
          preventDefault(event);
          break;
        case 13:
          historyIdx = -1;

          const cmd = (storage + term.line).trim();
          if (cmd == "cls") {
            term.cls();
            break;
          }

          var balance = Module.paren_balance(term.line);

          if (event.shiftKey || parens + balance != 0) {
            term.prompt = cont_prompt;
            storage += term.breakLine();
            parens += balance;
            break;
          }

          const lines = 
            Module.parse_eval(storage + term.line, storage.length != 0).split('\n');
          storage = "";
          parens = 0;
          term.prompt = prompt;
          term.newLine();
          for (let i = 0; i < lines.length; ++i) {
            // sigh
            term.echo(
              (lines.length > 1 && !i && lines[i].indexOf('^') != -1 
                ? ' '.repeat(term.prompt.length)
                : '')
              + lines[i])
          }
          break;
        case 85:
          if (event.ctrlKey) {
            term.clear();
            historyIdx = -1;
            preventDefault(event);
            return false;
          }
          break;
        case 37:
          term.jump(-1);
          preventDefault(event);
          break;
        case 39:
          term.jump(1);
          preventDefault(event);
          break;
        case 38:
          if (historyIdx == -1) {
            historyIdx = Module.history.size();
          }
          --historyIdx;
          if (historyIdx != -1) {
            term.clear();
            term.append(Module.history.get(historyIdx));
          } else if (Module.history.size()) {
            historyIdx = 0;
          }
          preventDefault(event);
          break;
        case 40:
          if (historyIdx != -1) {
            if (++historyIdx < Module.history.size()) {
              term.clear();
              term.append(Module.history.get(historyIdx));
            } else {
              --historyIdx;
            }
          }
          preventDefault(event);
          break;
        case 35:
          term.jump(1000000);
          preventDefault(event);
          break;
        case 36:
          term.jump(-1000000);
          preventDefault(event);
          break;
        default:
          break;
      }
    }
  });



}, 500);
