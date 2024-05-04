const express = require('express');
const bodyParser = require('body-parser');
const path = require('path');
const { checkUserPrivilege } = require('./build/Release/checkPrivilegeAddon.node');
const app = express();

app.set('view engine', 'ejs');
app.set('views', path.join(__dirname, 'views'));

app.use(express.static(path.join(__dirname, 'public')));
app.use(bodyParser.urlencoded({ extended: true }));
//app.use(bodyParser.json());

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.post('/check', (req, res) => {
    //const { username } = req.body;
    const username = req.body.username;
    console.log(username)
    const privilege = checkUserPrivilege(username);
    console.log(username, privilege)
    if (privilege) {
        res.render('privilege', { username, privilege });
    } else {
        res.render('error', { username });
    }
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    console.log(`Server is running on http://localhost:${PORT}`);
});