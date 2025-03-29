#!/usr/bin/python3
# -*- coding: utf-8 -*-
import cgi
import random

form = cgi.FieldStorage()
name = form.getfirst('value1') or "匿名A"

def fortune(name):
    try:
        omikuji = []
        for i in range(100):
            if i < 5:
                omikuji.append("大吉")
            elif i < 15:
                omikuji.append("中吉")
            elif i < 30:
                omikuji.append("小吉")
            elif i < 50:
                omikuji.append("吉")
            elif i < 65:
                omikuji.append("末吉")
            elif i < 75:
                omikuji.append("凶")
            elif i < 90:
                omikuji.append("中凶")
            else:
                omikuji.append("大凶")

        result = f"{name}さんの運勢は「{random.choice(omikuji)}」です！"
        return result
    except ValueError:
        return '失敗しました ><'

print("Content-Type: text/html; charset=UTF-8")

print()
try:
    htmlText = '''
    <!DOCTYPE html>
    <html>
        <head><meta charset="UTF-8" /></head>
    <body bgcolor="lightyellow">
        <h1>こんにちは</h1>
        <p>おみくじの結果は...&nbsp; %s<br/></p>
        <hr/>
        <a href="../index1.html">戻る</a>
    </body>
    </html>
    ''' % (fortune(name))
    print(htmlText)
except Exception as e:
    print("<html><body><h1>エラー発生</h1><p>{}</p></body></html>".format(str(e)))
