#!/usr/bin/python3
# -*- coding: utf-8 -*-
import cgi

form = cgi.FieldStorage()
v1 = form.getfirst('value1') or "0"
v2 = form.getfirst('value2') or "0"

def times(a, b):
    try:
        a = int(a)
        b = int(b)
        return str(a * b)
    except ValueError:
        return '数値ではないので計算できませんでした ><'

print("Content-Type: text/html; charset=UTF-8")

print()
try:
    htmlText = '''
    <!DOCTYPE html>
    <html>
        <head><meta charset="UTF-8" /></head>
    <body bgcolor="lightyellow">
        <h1>こんにちは</h1>
        <p>入力値の積は&nbsp; %s<br/></p>
        <hr/>
        <a href="../index1.html">戻る</a>
    </body>
    </html>
    ''' % (times(v1, v2))
    print(htmlText)
except Exception as e:
    print("<html><body><h1>エラー発生</h1><p>{}</p></body></html>".format(str(e)))