--[[********************************************************************
	Copyright (c) 2013-2015 Mogara

  This file is part of QSanguosha-Hegemony.

  This game is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 3.0
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  See the LICENSE file for more details.

  Mogara
*********************************************************************]]

---荀攸
local qice_skill = {}
qice_skill.name = "qice"
table.insert(sgs.ai_skills, qice_skill)
qice_skill.getTurnUseCard = function(self,room,player,data)
	if self.player:hasUsed("ViewAsSkill_qiceCard") or self.player:isKongcheng() then return end
	local handcardnum = self.player:getHandcardNum()
	local caocao = self.room:findPlayerBySkillName("jianxiong")
	local preventdamaget = false
	if not (caocao and self:isFriend(caocao)
		and caocao:getHp() > 1 
		and (caocao:hasShownSkill("jianxiong") or caocao:hasShownSkill("jjianxiong"))
		and not self:willSkipPlayPhase(caocao))
		and getCardsNum("Peach", self.player) > 0 then
		preventdamage = true
	end
	
	local usevalue = 0
	local keepvalue = 0	
	local id
	local cards = self.player:getHandcards()
	local allcard = {}
	cards = sgs.QList2Table(cards)
	for _,card in ipairs(cards) do
		if card:canRecast() then return end
		if card:isAvailable(self.player) then
			if card:isKindOf("EquipCard") and not self:getSameEquip(card) and handcardnum > 1 then
				local use = sgs.CardUseStruct(card, self.player, self.player, true)
				self:useEquipCard(card, use)
			end
			usevalue = self:getUseValue(card) + usevalue
		end
		if not id then
			id = tostring(card:getId())
		else
			id = id .. "+" .. tostring(card:getId())
		end
	end	
	self:sortByKeepValue(cards)
	for i = 1, #cards, 1 do
		if i > self:getOverflow(self.player) then
			keepvalue = self:getKeepValue(cards[i]) + keepvalue
		end
	end

	local cardsavailable = function(use_card)
		local targets = {}
		if use_card:isKindOf("AwaitExhausted") then
			for _, p in sgs.qlist(self.room:getAlivePlayers()) do
				if not self.player:isProhibited(p, use_card) and self.player:hasShownOneGeneral() and self.player:getRole() ~= "careerist" and
							p:getRole() ~= "careerist" and p:hasShownOneGeneral() and p:getKingdom() == self.player:getKingdom() then
						table.insert(targets, p)
				end
			end
		elseif use_card:getSubtype() == "global_effect" and not use_card:isKindOf("FightTogether") then
			for _, p in sgs.qlist(self.room:getAlivePlayers()) do
				if not self.player:isProhibited(p, use_card) then
					table.insert(targets, p)
				end
			end
		elseif use_card:getSubtype() == "aoe" and not use_card:isKindOf("BurningCamps") then
			for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
				if not self.player:isProhibited(p, use_card) then
					table.insert(targets, p)
				end
			end
		elseif use_card:isKindOf("BurningCamps") then
				players = self.player:getNextAlive():getFormation()
			for _, p in sgs.qlist(players) do
				if not self.player:isProhibited(p, use_card) then
					table.insert(targets, p)
				end
			end
		end
		return #targets <= handcardnum
	end
	
	local parsed_card = {}
	if cardsavailable(sgs.Card_Parse("amazing_grace:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("amazing_grace:qice[to_be_decided:0]=" .. id .."&qice"))		--五谷
	end
	if cardsavailable(sgs.Card_Parse("god_salvation:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("god_salvation:qice[to_be_decided:0]=" .. id .."&qice"))		--桃园
	end
	if cardsavailable(sgs.Card_Parse("burning_camps:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("burning_camps:qice[to_be_decided:0]=" .. id .."&qice"))		--火烧连营
	end
	if cardsavailable(sgs.Card_Parse("drowning:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("drowning:qice[to_be_decided:0]=" .. id .."&qice"))				--水淹七军
	end
	if cardsavailable(sgs.Card_Parse("threaten_emperor:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("threaten_emperor:qice[to_be_decided:0]=" .. id .."&qice"))		--挟天子以令诸侯
	end
	if cardsavailable(sgs.Card_Parse("await_exhausted:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("await_exhausted:qice[to_be_decided:0]=" .. id .."&qice"))			--以逸待劳
	end
	if cardsavailable(sgs.Card_Parse("befriend_attacking:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("befriend_attacking:qice[to_be_decided:0]=" .. id .."&qice"))		--远交近攻
	end
	if cardsavailable(sgs.Card_Parse("duel:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("duel:qice[to_be_decided:0]=" .. id .."&qice"))				--决斗
	end
	if cardsavailable(sgs.Card_Parse("dismantlement:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("dismantlement:qice[to_be_decided:0]=" .. id .."&qice"))		--过河拆桥
	end
	if cardsavailable(sgs.Card_Parse("snatch:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("snatch:qice[to_be_decided:0]=" .. id .."&qice"))				--顺手牵羊
	end
	if cardsavailable(sgs.Card_Parse("ex_nihilo:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("ex_nihilo:qice[to_be_decided:0]=" .. id .."&qice"))			--无中生有
	end
	if not preventdamage or not self:hasTrickEffective(sgs.Card_Parse("archery_attack:qice[to_be_decided:0]=" .. id .."&qice"), caocao)
		and cardsavailable(sgs.Card_Parse("archery_attack:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("archery_attack:qice[to_be_decided:0]=" .. id .."&qice"))	--万箭齐发
	end
	if not preventdamage or not self:hasTrickEffective(sgs.Card_Parse("savage_assault:qice[to_be_decided:0]=" .. id .."&qice"), caocao) and
		cardsavailable(sgs.Card_Parse("savage_assault:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("savage_assault:qice[to_be_decided:0]=" .. id .."&qice"))	--南蛮
	end
	
	local value = 0
	local tcard
	for _, c in ipairs(parsed_card) do
		assert(c)
		if self:getUseValue(c) > value and self:getUseValue(c) > keepvalue and self:getUseValue(c) > usevalue and (usevalue < 6 or handcardnum == 1) then
			value = self:getUseValue(c)
			tcard = c
		end
	end
	if tcard then
		return tcard
	end
end

sgs.ai_skill_invoke.zhiyu = function(self, data)
	if not self:willShowForMasochism() then return false end
	local damage = data:toDamage()
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	local first
	local difcolor = 0
	for _, card in ipairs(cards)  do
		if not first then first = card end
		if (first:isRed() and card:isBlack()) or (card:isRed() and first:isBlack()) then
			difcolor = 1
			break
		end
	end
	if difcolor == 0 and damage.from then
		if self:isFriend(damage.from) and not damage.from:isKongcheng() then
			return false
		elseif self:isEnemy(damage.from) then
			if self:doNotDiscard(damage.from, "h") and not damage.from:isKongcheng() then return false end
			return true
		end
	end
	return true
end

--卞皇后
--[[
sgs.ai_skill_invoke.wanwei = function(self, data)
	if not self:willShowForDefence() then return false end
	local move = data:toMoveOneTime()
	local target = move.to
	if self:isFriend(target) then return false end
	self.wanwei = {}
	local cards = sgs.QList2Table(self.player:getCards("e"))
	local handcards = sgs.QList2Table(self.player:getHandcards())
	table.insertTable(cards, handcards)
	self:sortByUseValue(cards)
	for i = 1, move.card_ids:length(), 1 do
		table.insert(self.wanwei, card[i]:getEffectiveId())
	end
	return true
end
--]]

sgs.ai_skill_exchange.wanwei = function(self)
	if not self:willShowForDefence() then return {} end
	local target = self.player:getTag("wanwei"):toPlayer()
	if not self:isFriend(target) then
		return self:askForDiscard("dummy_reason", 1, 1, false, true)
	else
		if self:isWeak(target) and not self:isWeak() and self:getCardsNum("Peach") > 0 then
			for _, c in sgs.qlist(self.player:getHandcards()) do
				if c:isKindOf("Peach") then return { c:getEffectiveId() } end
			end
		end
	end
	return {}
end

sgs.ai_skill_discard.wanwei = function(self)
	if not self:willShowForDefence() then return {} end
	return self:askForDiscard("dummy_reason", 1, 1, false, true)
end

sgs.ai_skill_invoke.yuejian = function(self, data)
	local target = self.room:getCurrent()
	if target:getHandcardNum() > target:getMaxCards() and target:isWounded() then return true end
	return false
end

--李郭
sgs.ai_skill_invoke.xichou = function(self, data)
	return false
end

--左慈
sgs.ai_skill_invoke.huashen = function(self, data)
	local huashens = self.player:getTag("Huashens"):toList()
	if huashens:length() < 2 then return true end
	local names = {}
	for _, q in sgs.qlist(huashens) do
		table.insert(names, q:toString())
	end
	g1 = sgs.Sanguosha:getGeneral(names[1])
	g2 = sgs.Sanguosha:getGeneral(names[2])
	return self:getHuashenPairValue(g1, g2) < 6
end

sgs.ai_skill_invoke["xinsheng"] = function(self, data)
	if self.player:ownSkill("xichou") and not self.player:hasShownSkill("huashen") and self.player:getMark("xichou") == 0 then return false end
	return true
end

sgs.ai_skill_choice.huashen = function(self, choice, data)
	local head = self.player:inHeadSkills("huashen") or self.player:inHeadSkills("xinsheng")
	local names = choice:split("+")
	local max_point = 0
	local pair = ""

	for _, name1 in ipairs(names) do
		local g1 = sgs.Sanguosha:getGeneral(name1)
		if not g1 then continue end
		for _, skill in sgs.qlist(g1:getVisibleSkillList(true, head)) do
			if skill:getFrequency() == sgs.Skill_Limited and skill:getLimitMark() ~= "" and self.player:getMark(skill:getLimitMark()) == 0 and self.player:hasSkill(skill:objectName()) then
				ajust = ajust - 1
			end
		end

		for _, name2 in ipairs(names) do
			local g2 = sgs.Sanguosha:getGeneral(name2)
			if not g2 or g1:getKingdom() ~= g2:getKingdom() or name1 == name2 then continue end
			local points = self:getHuashenPairValue(g1, g2)
			max_point = math.max(max_point, points)
			if max_point == points then pair = name1 .. "+" .. name2 end
		end
	end
	self.player:speak("结果是：" .. pair)
	return pair
end

function SmartAI:getHuashenPairValue(g1, g2)
	local player= self.player
	local current_value = 0
	for name, value in pairs(sgs.general_pair_value) do
		if g1:objectName() .. "+" .. g2:objectName() == name or g2:objectName() .. "+" .. g1:objectName() == name then
			current_value = value
			break
		end
	end
	local oringin_g1 = 3
	local oringin_g2 = 3
	for name, value in pairs(sgs.general_value) do
		if g1:objectName() == name then oringin_g1 = value end
		if g2:objectName() == name then oringin_g2 = value end
	end

	if current_value == 0 then
		local oringin_g1 = 3
		local oringin_g2 = 3
		for name, value in pairs(sgs.general_value) do
			if g1:objectName() == name then oringin_g1 = value end
			if g2:objectName() == name then oringin_g2 = value end
		end
		current_value = oringin_g1 + oringin_g2
	end

	local skills = {}
	for _, skill in sgs.qlist(g1:getVisibleSkillList(true, player:inHeadSkills("huashen"))) do
		table.insert(skills, skill:objectName())
		if skill:getFrequency() == sgs.Skill_Limited and skill:getLimitMark() ~= "" and player:getMark(skill:getLimitMark()) == 0 then
			current_value = current_value - 1
		end
	end
	for _, skill in sgs.qlist(g2:getVisibleSkillList(true, player:inHeadSkills("huashen"))) do
		table.insert(skills, skill:objectName())
		if skill:getFrequency() == sgs.Skill_Limited and skill:getLimitMark() ~= "" and player:getMark(skill:getLimitMark()) == 0 then
			current_value = current_value - 1
		end
	end

	if g1:isCompanionWith(g2:objectName()) and player:getMark("CompanionEffect") == 0 then
		current_value = current_value - 0.5
	end

	for _, skill in ipairs(skills) do
		if sgs.cardneed_skill:match(skill) then
			if player:getHandcardNum() < 3 then
				current_value = current_value - 0.4
			elseif player:getHandcardNum() < 5 then
				current_value = current_value + 0.5
			end
		end
		if sgs.masochism_skill:match(skill) then
			if player:getHp() < 2 then
				current_value = current_value - 0.3
			end
			for i = 1, player:getHp() - 3, 1 do
				current_value = current_value + 0.6
			end
			for i = 1, self:getCardsNum("Peach"), 1 do
				current_value = current_value + 0.15
			end
			for i = 1, self:getCardsNum("Analeptic"), 1 do
				current_value = current_value + 0.1
			end
		end
		if sgs.lose_equip_skill:match(skill) then
			if self:getCardsNum("EquipCard") < 2 then
				current_value = current_value - 0.3
			end
			for i = 1, self:getCardsNum("EquipCard"), 1 do
				current_value = current_value + 0.1
			end
			for _, p in sgs.qlist(self.room:getOtherPlayers(player)) do
				if self:isFriend(p) then
					if p:hasShownSkill("duoshi") then
						current_value = current_value + 0.4
					end
					if p:hasShownSkill("zhijian") then
						current_value = current_value + 0.3
					end
				end
			end
		end
		if skill == "jizhi" then
			for i = 1, self:getCardsNum("TrickCard"), 1 do
				current_value = current_value + 0.1
			end
		end
	end
	player:speak(g1:objectName() .. "+" .. g2:objectName() .. "的组合得分是：" .. current_value)
	return current_value
end

sgs.ai_skill_choice.xinsheng = function(self, choice, data)
	return sgs.ai_skill_choice["huashen"](self, choice, data)
end

--沙摩柯
sgs.ai_skill_invoke.jili = function(self, data)
	if not self:willShowForAttack() then return false end
	return true
end

--马谡
sgs.ai_skill_invoke.zhiman = function(self, data)
	local damage = self.player:getTag("zhiman_data"):toDamage()
	local target = damage.to
	local promo = self:findPlayerToDiscard("ej", false, sgs.Card_MethodGet, nil, true)
	if self:isFriend(damage.to) and (table.contains(promo, target) or not self:needToLoseHp(target, self.player)) then return true end
	if not self:isFriend(damage.to) and damage.damage > 1 and not target:hasArmorEffect("SilverLion") then return false end
	if table.contains(promo, target) then return true end
	if target:hasShownSkills(sgs.masochism_skill) and self.player:canGetCard(target, "e") then self.player:speak("因为防止卖血") return true end
	return false
end

sgs.ai_choicemade_filter.skillInvoke.zhiman = function(self, player, promptlist)
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	if damage.from and damage.to then
		if promptlist[#promptlist] == "yes" then
			if not damage.to:hasEquip() and damage.to:getJudgingArea():isEmpty() then
				sgs.updateIntention(damage.from, damage.to, -40)
			end
		elseif self:canAttack(damage.to) then
			sgs.updateIntention(damage.from, damage.to, 30)
		end
	end
end

sgs.ai_choicemade_filter.cardChosen.zhiman = function(self, player, promptlist)
	local intention = 10
	local id = promptlist[3]
	local card = sgs.Sanguosha:getCard(id)
	local target = findPlayerByObjectName(promptlist[5])
	if self:needToThrowArmor(target) and self.room:getCardPlace(id) == sgs.Player_PlaceEquip and card:isKindOf("Armor") then
		intention = -intention
	elseif self:doNotDiscard(target) then intention = -intention
	elseif target:hasShownSkills(sgs.lose_equip_skill) and not target:getEquips():isEmpty() and
		self.room:getCardPlace(id) == sgs.Player_PlaceEquip and card:isKindOf("EquipCard") then
			intention = -intention
	elseif self.room:getCardPlace(id) == sgs.Player_PlaceJudge then
		intention = -intention
	end
	sgs.updateIntention(player, target, intention)
end

local sanyao_skill = {}
sanyao_skill.name = "sanyao"
table.insert(sgs.ai_skills, sanyao_skill)
sanyao_skill.getTurnUseCard = function(self, room, player, data)
	if not self:willShowForAttack() then return end
	if self.player:hasUsed("SanyaoCard") then return end
	if self.player:isNude() then return end
	return sgs.Card_Parse("@SanyaoCard=.&sanyao")
end

sgs.ai_skill_use_func.SanyaoCard = function(card, use, self)
	local targets = sgs.SPlayerList()
	local maxhp = 0
    for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if maxhp < p:getHp() then maxhp = p:getHp() end
	end
    for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:getHp() == maxhp then
			targets:append(p)
		end
	end
	if self:isWeak() or not not self:needToLoseHp() then
		targets:removeOne(self.player)
	end
	local target = self:findPlayerToDiscard("ej", false, sgs.Card_MethodGet, targets)
	if not target then
		for _, p in sgs.qlist(targets) do
			if self:isEnemy(p) and self:isWeak(p) then target = p break end
		end
	end
	if not target then
		for _, p in sgs.qlist(targets) do
			if self:getDangerousCard(p) and self.player:canGetCard(p, self:getDangerousCard(p)) then
				target = p
				break
			end
		end
	end
	if not target then
		for _, p in sgs.qlist(targets) do
			if self:isEnemy(p) and not p:hasShownSkills(sgs.masochism_skill) and self:getOverflow() > 0 then target = p break end
		end
	end

	if self:needToThrowArmor() then
		use.card = sgs.Card_Parse("@SanyaoCard=" .. self.player:getArmor():getId() .. "&sanyao")
		if targets:length() == 0 then use.card = nil return end
		if use.to then
			if target then
				use.to:append(target)
			else
				use.to:append(targets:first())
			end
			return
		end
	else
		if not target then use.card = nil return end
		local cards = sgs.QList2Table(self.player:getCards("e"))
		for _, c in sgs.qlist(self.player:getHandcards()) do
			table.insert(cards, c)
		end
		self:sortByUseValue(cards)

		local card_id
		for _, c in ipairs(cards) do
			if c:isKindOf("Lightning") and not isCard("Peach", c, self.player) and not self:willUseLightning(c) then
				card_id = c:getEffectiveId()
				break
			end
		end

		if not card_id then
			for _, c in ipairs(cards) do
				if not isCard("Peach", c, self.player)
					and (c:isKindOf("AmazingGrace") or c:isKindOf("GodSalvation") and not self:willUseGodSalvation(c)) then
					card_id = c:getEffectiveId()
					break
				end
			end
		end
		if not card_id then
			for _, c in ipairs(cards) do
				if (not isCard("Peach", c, self.player) or self:getCardsNum("Peach") > 1)
					and (not isCard("Jink", c, self.player) or self:getCardsNum("Jink") > 1 or self:isWeak())
					and not (self.player:getWeapon() and self.player:getWeapon():getEffectiveId() == c:getEffectiveId())
					and not (self.player:getOffensiveHorse() and self.player:getOffensiveHorse():getEffectiveId() == c:getEffectiveId()) then
					card_id = c:getEffectiveId()
				end
			end
		end
		if card_id then
			use.card = sgs.Card_Parse("@SanyaoCard=" .. card_id .. "&sanyao")
			if use.to then
				if use.to:isEmpty() then use.to:append(target) return end
			end
		end
	end
end

--凌统
sgs.ai_skill_playerchosen.liefeng = function(self, targets)
	if not self:willShowForAttack() then return nil end
	return self:findPlayerToDiscard()
end

sgs.ai_choicemade_filter.cardChosen.liefeng = function(self, player, promptlist)
	local intention = 10
	local id = promptlist[3]
	local card = sgs.Sanguosha:getCard(id)
	local target = findPlayerByObjectName(promptlist[5])
	if self:needToThrowArmor(target) and self.room:getCardPlace(id) == sgs.Player_PlaceEquip and card:isKindOf("Armor") then
		intention = -intention
	elseif self:doNotDiscard(target) then intention = -intention
	elseif target:hasShownSkills(sgs.lose_equip_skill) and not target:getEquips():isEmpty() and
		self.room:getCardPlace(id) == sgs.Player_PlaceEquip and card:isKindOf("EquipCard") then
			intention = -intention
	elseif self:getOverflow(target) > 2 then intention = 0
	end
	sgs.updateIntention(player, target, intention)
end

xuanlue_skill = {}
xuanlue_skill.name = "xuanlue"
table.insert(sgs.ai_skills, xuanlue_skill)
xuanlue_skill.getTurnUseCard = function(self)
	if self.player:getMark("@lue") <= 0 then return end
	if not self:willShowForAttack() then return end
	local targets = {}
	self.resultcard = {}
	self.resulttargets = {}
	self.gotcard = {}
	local danger = {}
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		p:setFlags("-xuanlue_gotweapon")
		p:setFlags("-xuanlue_gotarmor")
		p:setFlags("-xuanlue_gotoffensivehorse")
		p:setFlags("-xuanlue_gotdefensivehorse")
		p:setFlags("-xuanlue_gottreasure")
		if p:objectName() == self.player:objectName() then continue end
		if self.player:canGetCard(p, "e") then
			table.insert(targets, p)
		end
	end

	for _, p in ipairs(targets) do
		if not self:isFriend(p) then
			local dangerous = self:getDangerousCard(p)
			if dangerous and self.player:canGetCard(p, dangerous) then
				local card = sgs.Sanguosha:getCard(dangerous)
				table.insert(danger, card)
			end
			local valuable = self:getValuableCard(p)
			if valuable and self.player:canGetCard(p, valuable) and not table.contains(danger, sgs.Sanguosha:getCard(valuable)) then
				table.insert(danger, sgs.Sanguosha:getCard(valuable))
			end
		end
	end
	for _, p in ipairs(targets) do
		if not self:isFriend(p) then
			for _, eq in sgs.qlist(p:getEquips()) do
				if self.player:canGetCard(p, eq:getEffectiveId()) and not table.contains(danger, eq) then
					table.insert(danger, eq)
				end
			end
		end
	end

	for _, p in ipairs(self.friends) do
		if self:isFriend(p) and p:hasSkills(sgs.need_equip_skill .. "|" .. sgs.lose_equip_skill) then
			for _, card in ipairs(danger) do
				if (card:isKindOf("Weapon") and not p:getWeapon() and not p:hasFlag("xuanlue_gotweapon")) then
					p:setFlags("xuanlue_gotweapon")
					if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
					if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
				end
				if (card:isKindOf("Armor") and not p:getArmor() and not p:hasFlag("xuanlue_gotarmor")) then
					p:setFlags("xuanlue_gotarmor")
					if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
					if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
				end
				if (card:isKindOf("OffensiveHorse") and not p:getOffensiveHorse() and not p:hasFlag("xuanlue_gotoffensivehorse")) then
					p:setFlags("xuanlue_gotoffensivehorse")
					if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
					if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
				end
				if (card:isKindOf("DefensiveHorse") and not p:getDefensiveHorse() and not p:hasFlag("xuanlue_gotdefensivehorse")) then
					p:setFlags("xuanlue_gotdefensivehorse")
					if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
					if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
				end
				if (card:isKindOf("Treasure") and not p:getTreasure() and not p:hasFlag("xuanlue_gottreasure")) then
					p:setFlags("xuanlue_gottreasure")
					if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
					if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
				end
				if #self.resultcard >= #danger or #self.resultcard >= 3 then break end
			end
			for _, card in ipairs(self.resultcard) do
				table.removeOne(danger, card)
			end
		end
		if #self.resultcard >= #danger or #self.resultcard >= 3 then break end
	end

	if #self.resultcard < 3 and #self.resultcard < #danger then
		for _, p in ipairs(self.friends) do
			if self:isFriend(p) then
				for _, card in ipairs(danger) do
					if (card:isKindOf("Weapon") and not p:getWeapon() and not p:hasFlag("xuanlue_gotweapon")) then
						p:setFlags("xuanlue_gotweapon")
						if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
						if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
					end
					if (card:isKindOf("Armor") and not p:getArmor() and not p:hasFlag("xuanlue_gotarmor")) then
						p:setFlags("xuanlue_gotarmor")
						if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
						if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
					end
					if (card:isKindOf("OffensiveHorse") and not p:getOffensiveHorse() and not p:hasFlag("xuanlue_gotoffensivehorse")) then
						p:setFlags("xuanlue_gotoffensivehorse")
						if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
						if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
					end
					if (card:isKindOf("DefensiveHorse") and not p:getDefensiveHorse() and not p:hasFlag("xuanlue_gotdefensivehorse")) then
						p:setFlags("xuanlue_gotdefensivehorse")
						if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
						if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
					end
					if (card:isKindOf("Treasure") and not p:getTreasure() and not p:hasFlag("xuanlue_gottreasure")) then
						p:setFlags("xuanlue_gottreasure")
						if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
						if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
					end
					if #self.resultcard >= #danger or #self.resultcard >= 3 then break end
				end
				for _, card in ipairs(self.resultcard) do
					table.removeOne(danger, card)
				end
			end
			if #self.resultcard >= #danger or #self.resultcard >= 3 then break end
		end
	end

	if #self.resultcard >= 3 then
		sgs.ai_use_priority.XuanlueCard = 9
		return sgs.Card_Parse("@XuanlueCard=.&xuanlue")
	else
		for _, p in ipairs(self.friends_noself) do
			if self:needToThrowArmor(p) and self.player:canGetCard(p, p:getArmor():getEffectiveId()) then
				if not table.contains(self.resultcard, p:getArmor()) then table.insert(self.resultcard, p:getArmor()) end
				if not table.contains(self.resulttargets, p) then table.insert(self.resulttargets, p) end
			end
			if p:hasShownSkills(sgs.lose_equip_skill) and p:hasEquip() then
				for _, eq in sgs.qlist(p:getEquips()) do
					if self.player:canGetCard(p, eq:getEffectiveId()) then
						if not table.contains(self.resultcard, eq) then table.insert(self.resultcard, eq) end
						if not table.contains(self.resulttargets, p) then table.insert(self.resulttargets, p) end
					end
				end
			end
		end
	end
	if #self.resultcard >= 3 then
		sgs.ai_use_priority.XuanlueCard = 9
		return sgs.Card_Parse("@XuanlueCard=.&xuanlue")
	else return end
end

sgs.ai_skill_use_func.XuanlueCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_playerchosen.xuanlue = function(self, targets)
	local targetlist = sgs.QList2Table(targets)
	for _, p in ipairs(targetlist) do
		if not self:isFriend(p) and table.contains(self.resulttargets, p) then
			for _, eq in sgs.qlist(p:getEquips()) do
				if table.contains(self.resultcard, eq) and not table.contains(self.gotcard, eq) then
					return p
				end
			end
		end
	end
	for _, p in ipairs(targetlist) do
		if table.contains(self.resulttargets, p) then
			for _, eq in sgs.qlist(p:getEquips()) do
				if table.contains(self.resultcard, eq) and not table.contains(self.gotcard, eq) then
					return p
				end
			end
		end
	end
end

sgs.ai_skill_cardchosen.xuanlue = function(self, p)
	for _, eq in sgs.qlist(p:getEquips()) do
		if table.contains(self.resultcard, eq) and not table.contains(self.gotcard, eq) then
			table.insert(self.gotcard, eq)
			return eq:getEffectiveId()
		end
	end
end

sgs.ai_skill_use["@@xuanlue_equip!"] = function(self, prompt)
	local card_ids = self.player:property("xuanlue_cards"):toString():split("+")
	for _, id in ipairs(card_ids) do
		local card = sgs.Sanguosha:getCard(id)
		for _, p in ipairs(self.friends) do
			if card:isKindOf("Weapon") and p:hasFlag("xuanlue_gotweapon") then
				p:setFlags("-xuanlue_gotweapon")
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("Armor") and p:hasFlag("xuanlue_gotarmor") then
				p:setFlags("-xuanlue_gotarmor")
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("OffensiveHorse") and p:hasFlag("xuanlue_gotoffensivehorse") then
				p:setFlags("-xuanlue_gotoffensivehorse")
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("DefensiveHorse") and p:hasFlag("xuanlue_gotdefensivehorse") then
				p:setFlags("-xuanlue_gotdefensivehorse")
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("Treasure") and p:hasFlag("xuanlue_gottreasure") then
				p:setFlags("-xuanlue_gottreasure")
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
		end
	end
	for _, id in ipairs(card_ids) do
		local card = sgs.Sanguosha:getCard(id)
		for _, p in ipairs(self.friends) do
			if card:isKindOf("Weapon") and not p:getWeapon() then
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("Armor") and not p:getArmor() then
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("OffensiveHorse") and not p:getOffensiveHorse() then
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("DefensiveHorse") and not p:getDefensiveHorse() then
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("Treasure") and not p:getTreasure() then
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
		end
	end
	return "."
end

--吕范
local diaodu_skill = {}
diaodu_skill.name = "diaodu"
table.insert(sgs.ai_skills, diaodu_skill)
diaodu_skill.getTurnUseCard = function(self,room,player,data)
	if self.player:hasUsed("DiaoduCard") then return end
	if #self.friends_noself == 0 then return end
	return sgs.Card_Parse("@DiaoduCard=.&diaodu")
end

sgs.ai_skill_use_func.DiaoduCard = function(card, use, self)
	use.card = nil
	for _, card in sgs.qlist(self.player:getCards("h")) do
		if card:isKindOf("Weapon") and not self.player:getWeapon() then
			use.card = card
		end
		if card:isKindOf("Armor") and not self.player:getArmor() then
			use.card = card
		end
		if card:isKindOf("OffensiveHorse") and not self.player:getOffensiveHorse() then
			use.card = card
		end
		if card:isKindOf("DefensiveHorse") and not self.player:getDefensiveHorse() then
			use.card = card
		end
		if card:isKindOf("Treasure") and not self.player:getTreasure() then
			use.card = card
		end
		if use.card then return end
	end
	use.card = card
end

sgs.ai_skill_use["@@diaodu_equip"] = function(self, prompt)
	local id

	if self:needToThrowArmor() then
		for _, p in ipairs(self.friends_noself) do
			if self.player:isFriendWith(p) and not p:getArmor() then
				return ("@DiaoduequipCard=" .. self.player:getArmor():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
			end
		end
	end
	for _, p in ipairs(self.friends_noself) do
		if not self.player:isFriendWith(p) then continue end
		for _, card in sgs.qlist(self.player:getCards("h")) do
			if card:isKindOf("Weapon") and self.player:getWeapon() then
				return "@DiaoduequipCard=" .. self.player:getWeapon():getEffectiveId() .. "&diaoduequip->" .. p:objectName()
			end
			if card:isKindOf("Armor") and self.player:getArmor() and not (self.player:getArmor():isKindOf("PeaceSpell") and self:isWeak()) then
				return "@DiaoduequipCard=" .. self.player:getArmor():getEffectiveId() .. "&diaoduequip->" .. p:objectName()
			end
			if card:isKindOf("OffensiveHorse") and self.player:getOffensiveHorse() then
				return "@DiaoduequipCard=" .. self.player:getOffensiveHorse():getEffectiveId() .. "&diaoduequip->" .. p:objectName()
			end
			if card:isKindOf("DefensiveHorse") and self.player:getDefensiveHorse() then
				return "@DiaoduequipCard=" .. self.player:getDefensiveHorse():getEffectiveId() .. "&diaoduequip->" .. p:objectName()
			end
			if card:isKindOf("Treasure") and self.player:getTreasure() and (not self.player:getArmor():isKindOf("WoodenOx") and self.player:getPile("wooden_ox"):length() > 1) then
				return "@DiaoduequipCard=" .. self.player:getTreasure():getEffectiveId() .. "&diaoduequip->" .. p:objectName()
			end
		end
	end
	if self.player:hasSkills(sgs.lose_equip_skill) then
		for _, card in sgs.qlist(self.player:getCards("e")) do
			for _, p in ipairs(self.friends_noself) do
				if not self.player:isFriendWith(p) then continue end
				if card:isKindOf("Armor") and not p:getArmor() and not(card:isKindOf("PeaceSpell") and self:isWeak()) then
					return ("@DiaoduequipCard=" .. self.player:getArmor():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
				end
				if card:isKindOf("OffensiveHorse") and not p:getOffensiveHorse() then
					return ("@DiaoduequipCard=" .. self.player:getOffensiveHorse():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
				end
				if card:isKindOf("DefensiveHorse") and not p:getDefensiveHorse() then
					return ("@DiaoduequipCard=" .. self.player:getDefensiveHorse():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
				end
				if card:isKindOf("Treasure") and not p:getTreasure() and (not self.player:getArmor():isKindOf("WoodenOx") and self.player:getPile("wooden_ox"):length() > 1) then
					return ("@DiaoduequipCard=" .. self.player:getTreasure():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
				end
				if card:isKindOf("Weapon") and not p:getWeapon() then
					return ("@DiaoduequipCard=" .. self.player:getWeapon():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
				end
			end
		end
	end
	for _, card in sgs.qlist(self.player:getCards("e")) do
		for _, p in ipairs(self.friends_noself) do
			if not self.player:isFriendWith(p) or not p:hasShownSkills(sgs.need_equip_skill .. "|" .. sgs.lose_equip_skill) then continue end
			if card:isKindOf("Armor") and not p:getArmor() and not(card:isKindOf("PeaceSpell") and self:isWeak()) then
				return ("@DiaoduequipCard=" .. self.player:getArmor():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
			end
			if card:isKindOf("OffensiveHorse") and not p:getOffensiveHorse() then
				return ("@DiaoduequipCard=" .. self.player:getOffensiveHorse():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
			end
			if card:isKindOf("DefensiveHorse") and not p:getDefensiveHorse() then
				return ("@DiaoduequipCard=" .. self.player:getDefensiveHorse():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
			end
			if card:isKindOf("Treasure") and not p:getTreasure() and (not self.player:getArmor():isKindOf("WoodenOx") and self.player:getPile("wooden_ox"):length() > 1) then
				return ("@DiaoduequipCard=" .. self.player:getTreasure():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
			end
			if card:isKindOf("Weapon") and not p:getWeapon() then
				return ("@DiaoduequipCard=" .. self.player:getWeapon():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
			end
		end
	end

	for _, card in sgs.qlist(self.player:getCards("h")) do
		if card:isKindOf("EquipCard") then
			local dummy_use = {isDummy = true}
			self:useEquipCard(card, dummy_use)
			if dummy_use.card and dummy_use.card:getEffectiveId() == card:getEffectiveId() then
				return card:toString()
			end
		end
	end
	return "."
end

sgs.ai_skill_invoke.diancai = function(self, data)
	if not self:willShowForDefence() then return false end
	if self.player:getHandcardNum() < self.player:getMaxHp() then return true end
	return false
end

--孙权
local lianzi_skill = {}
lianzi_skill.name = "lianzi"
table.insert(sgs.ai_skills, lianzi_skill)
lianzi_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("LianziCard") or self.player:isKongcheng() then return end
	local num = 0
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:hasShownOneGeneral() and p:getKingdom() == "wu" and p:getRole() ~= "careerist" then
			num = num + p:getEquips():length()
		end
	end
	if num >= 3 then
		local handcards = sgs.QList2Table(self.player:getHandcards())
		self:sortByUseValue(handcards)
		return sgs.Card_Parse("@LianziCard=" .. handcards[1]:getEffectiveId() .. "&lianzi")
	end
end

sgs.ai_skill_use_func.LianziCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_movecards.lianzi = function(self, upcards, downcards, min_num, max_num)
	local ups = {}
	local down = {}
	for _, id in ipairs(upcards) do
		if sgs.Sanguosha:getCard(id):hasFlag("lianzi") then
			table.insert(down, id)
		else
			table.insert(ups, id)
		end
	end
	return ups, down
end

sgs.ai_skill_invoke.jubao = function(self, data)
	return true
end

--攻心
local gongxin_skill = {}
gongxin_skill.name = "gongxin"
table.insert(sgs.ai_skills,gongxin_skill)
gongxin_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("GongxinCard") then return end
	local gongxin_card = sgs.Card_Parse("@GongxinCard=.&showforviewhas")
	assert(gongxin_card)
	return gongxin_card
end

sgs.ai_skill_use_func.GongxinCard = function(card,use,self)
	self:sort(self.enemies, "handcard")
	self.enemies = sgs.reverse(self.enemies)

	for _, enemy in ipairs(self.enemies) do
		if not enemy:isKongcheng() and self:objectiveLevel(enemy) > 0
			and (self:hasSuit("heart", false, enemy) or self:getKnownNum(eneny) ~= enemy:getHandcardNum()) then
			use.card = card
			if use.to then
				use.to:append(enemy)
			end
			return
		end
	end
end

sgs.ai_skill_invoke.yingziextra = function(self, data)
	if not self:willShowForAttack() and not self:willShowForDefence() then
		return false
	end
	if self.player:hasFlag("haoshi") then
		local invoke = self.player:getTag("haoshi_yingziextra"):toBool()
		self.player:removeTag("haoshi_yingziextra")
		return invoke
	end
	return true
end

sgs.ai_skill_askforag.gongxin = function(self, card_ids)
	self.gongxinchoice = nil
	local target = self.player:getTag("gongxin"):toPlayer()
	if not target or self:isFriend(target) then return -1 end
	local nextAlive = self.player
	repeat
		nextAlive = nextAlive:getNextAlive()
	until nextAlive:faceUp()

	local peach, ex_nihilo, jink, nullification, slash
	local valuable
	for _, id in ipairs(card_ids) do
		local card = sgs.Sanguosha:getCard(id)
		if card:isKindOf("Peach") then peach = id end
		if card:isKindOf("ExNihilo") then ex_nihilo = id end
		if card:isKindOf("Jink") then jink = id end
		if card:isKindOf("Nullification") then nullification = id end
		if card:isKindOf("Slash") then slash = id end
	end
	valuable = peach or ex_nihilo or jink or nullification or slash or card_ids[1]
	local card = sgs.Sanguosha:getCard(valuable)
	if self:isEnemy(target) and target:hasSkill("tuntian") then
		local zhangjiao = self.room:findPlayerBySkillName("guidao")
		if zhangjiao and self:isFriend(zhangjiao, target) and self:canRetrial(zhangjiao, target) and self:isValuableCard(card, zhangjiao) then
			self.gongxinchoice = "discard"
		else
			self.gongxinchoice = "put"
		end
		return valuable
	end

	local willUseExNihilo, willRecast
	if self:getCardsNum("ExNihilo") > 0 then
		local ex_nihilo = self:getCard("ExNihilo")
		if ex_nihilo then
			local dummy_use = { isDummy = true }
			self:useTrickCard(ex_nihilo, dummy_use)
			if dummy_use.card then willUseExNihilo = true end
		end
	elseif self:getCardsNum("IronChain") > 0 then
		local iron_chain = self:getCard("IronChain")
		if iron_chain then
			local dummy_use = { to = sgs.SPlayerList(), isDummy = true }
			self:useTrickCard(iron_chain, dummy_use)
			if dummy_use.card and dummy_use.to:isEmpty() then willRecast = true end
		end
	end
	if willUseExNihilo or willRecast then
		local card = sgs.Sanguosha:getCard(valuable)
		if card:isKindOf("Peach") then
			self.gongxinchoice = "put"
			return valuable
		end
		if card:isKindOf("TrickCard") or card:isKindOf("Indulgence") or card:isKindOf("SupplyShortage") then
			local dummy_use = { isDummy = true }
			self:useTrickCard(card, dummy_use)
			if dummy_use.card then
				self.gongxinchoice = "put"
				return valuable
			end
		end
		if card:isKindOf("Jink") and self:getCardsNum("Jink") == 0 then
			self.gongxinchoice = "put"
			return valuable
		end
		if card:isKindOf("Nullification") and self:getCardsNum("Nullification") == 0 then
			self.gongxinchoice = "put"
			return valuable
		end
		if card:isKindOf("Slash") and self:slashIsAvailable() then
			local dummy_use = { isDummy = true }
			self:useBasicCard(card, dummy_use)
			if dummy_use.card then
				self.gongxinchoice = "put"
				return valuable
			end
		end
		self.gongxinchoice = "discard"
		return valuable
	end

	local hasLightning, hasIndulgence, hasSupplyShortage
	local tricks = nextAlive:getJudgingArea()
	if not tricks:isEmpty() then
	--if not tricks:isEmpty() and not nextAlive:containsTrick("YanxiaoCard") then
		local trick = tricks:at(tricks:length() - 1)
		if self:hasTrickEffective(trick, nextAlive) then
			if trick:isKindOf("Lightning") then hasLightning = true
			elseif trick:isKindOf("Indulgence") then hasIndulgence = true
			elseif trick:isKindOf("SupplyShortage") then hasSupplyShortage = true
			end
		end
	end

	if self:isEnemy(nextAlive) and nextAlive:hasSkill("luoshen") and valuable then
		self.gongxinchoice = "put"
		return valuable
	end
	if nextAlive:hasSkill("yinghun") and nextAlive:isWounded() then
		self.gongxinchoice = self:isFriend(nextAlive) and "put" or "discard"
		return valuable
	end
	if target:hasSkill("hongyan") and hasLightning and self:isEnemy(nextAlive) and not (nextAlive:hasSkill("qiaobian") and nextAlive:getHandcardNum() > 0) then
		for _, id in ipairs(card_ids) do
			local card = sgs.Sanguosha:getEngineCard(id)
			if card:getSuit() == sgs.Card_Spade and card:getNumber() >= 2 and card:getNumber() <= 9 then
				self.gongxinchoice = "put"
				return id
			end
		end
	end
	if hasIndulgence and self:isFriend(nextAlive) then
		self.gongxinchoice = "put"
		return valuable
	end
	if hasSupplyShortage and self:isEnemy(nextAlive) and not (nextAlive:hasSkill("qiaobian") and nextAlive:getHandcardNum() > 0) then
		local enemy_null = 0
		for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if self:isFriend(p) then enemy_null = enemy_null - getCardsNum("Nullification", p) end
			if self:isEnemy(p) then enemy_null = enemy_null + getCardsNum("Nullification", p) end
		end
		enemy_null = enemy_null - self:getCardsNum("Nullification")
		if enemy_null < 0.8 then
			self.gongxinchoice = "put"
			return valuable
		end
	end

	if self:isFriend(nextAlive) and not self:willSkipDrawPhase(nextAlive) and not self:willSkipPlayPhase(nextAlive)
		and not nextAlive:hasSkill("luoshen")
		and not nextAlive:hasSkill("tuxi") and not (nextAlive:hasSkill("qiaobian") and nextAlive:getHandcardNum() > 0) then
		if (peach and valuable == peach) or (ex_nihilo and valuable == ex_nihilo) then
			self.gongxinchoice = "put"
			return valuable
		end
		if jink and valuable == jink and getCardsNum("Jink", nextAlive) < 1 then
			self.gongxinchoice = "put"
			return valuable
		end
		if nullification and valuable == nullification and getCardsNum("Nullification", nextAlive) < 1 then
			self.gongxinchoice = "put"
			return valuable
		end
		if slash and valuable == slash and self:hasCrossbowEffect(nextAlive) then
			self.gongxinchoice = "put"
			return valuable
		end
	end

	local card = sgs.Sanguosha:getCard(valuable)
	local keep = false
	if card:isKindOf("Slash") or card:isKindOf("Jink")
		or card:isKindOf("EquipCard")
		or card:isKindOf("Disaster") or card:isKindOf("GlobalEffect") or card:isKindOf("Nullification")
		or target:isLocked(card) then
		keep = true
	end
	self.gongxinchoice = (target:objectName() == nextAlive:objectName() and keep) and "put" or "discard"
	return valuable
end

sgs.ai_skill_choice.gongxin = function(self, choices)
	return self.gongxinchoice or "discard"
end

sgs.ai_use_value.GongxinCard = 8.5
sgs.ai_use_priority.GongxinCard = 11
sgs.ai_card_intention.GongxinCard = 80

flamemap_skill = {}
flamemap_skill.name = "flamemap"
table.insert(sgs.ai_skills, flamemap_skill)
flamemap_skill.getTurnUseCard = function(self)
	local cards = {}
	for _, c in sgs.qlist(self.player:getCards("he")) do
		if c:isKindOf("EquipCard") then table.insert(cards, c) end
	end
	if #cards == 0 then return end
	if not self.player:hasUsed("FlameMapCard") then
		return sgs.Card_Parse("@FlameMapCard=.&showforviewhas")
	end
end

sgs.ai_skill_use_func.FlameMapCard = function(card,use,self)
	local sunquan = self.room:getLord(self.player:getKingdom())
	if not sunquan or not sunquan:hasLordSkill("jiahe") then return end
	local full = (sunquan:getPile("flame_map"):length() >= 4)
	sgs.ai_use_priority.FlameMapCard = 0
	if self.player:hasSkills(sgs.lose_equip_skill) then
		for _, card in sgs.qlist(self.player:getCards("h")) do
			if card:isKindOf("EquipCard") then return end
		end
		if self:needToThrowArmor() then
			--self.player:speak("有返回：" .. self.player:getArmor():getLogName())
			use.card = sgs.Card_Parse("@FlameMapCard=" .. self.player:getArmor():getEffectiveId() .. "&showforviewhas")
			return
		end
		if self.player:getOffensiveHorse() then
			--self.player:speak("有返回：" .. self.player:getOffensiveHorse():getLogName())
			use.card = sgs.Card_Parse("@FlameMapCard=" .. self.player:getOffensiveHorse():getEffectiveId() .. "&showforviewhas")
			return
		end
		if self.player:getWeapon() then
			--self.player:speak("有返回：" .. self.player:getWeapon():getLogName())
			use.card = sgs.Card_Parse("@FlameMapCard=" .. self.player:getWeapon():getEffectiveId() .. "&showforviewhas")
			return
		end
		if self.player:getArmor() and not(self.player:getArmor():isKindOf("PeaceSpell") and self:isWeak()) then
			--self.player:speak("有返回：" .. self.player:getArmor():getLogName())
			use.card = sgs.Card_Parse("@FlameMapCard=" .. self.player:getArmor():getEffectiveId() .. "&showforviewhas")
			return
		end
		if self.player:getDefensiveHorse() then
			--self.player:speak("有返回：" .. self.player:getDefensiveHorse():getLogName())
			use.card = sgs.Card_Parse("@FlameMapCard=" .. self.player:getDefensiveHorse():getEffectiveId() .. "&showforviewhas")
			return
		end
	else
		if self.player:hasEquip() then
			if self:needToThrowArmor() then
				sgs.ai_use_priority.FlameMapCard = 9
				--self.player:speak("有返回：" .. self.player:getArmor():getLogName())
				use.card = sgs.Card_Parse("@FlameMapCard=" .. self.player:getArmor():getEffectiveId() .. "&showforviewhas")
				return
			end
			if self.player:getArmor() and self:evaluateArmor(self.player:getArmor(), who) < -5 then
				sgs.ai_use_priority.FlameMapCard = 9
				--self.player:speak("有返回：" .. self.player:getArmor():getLogName())
				use.card = sgs.Card_Parse("@FlameMapCard=" .. self.player:getArmor():getEffectiveId() .. "&showforviewhas")
				return
			end
		end
		for _, card in sgs.qlist(self.player:getCards("h")) do
			if card:isKindOf("EquipCard") then
				local dummy_use = {isDummy = true}
				self:useEquipCard(card, dummy_use)
				if not dummy_use.card then
					if self.room:getCardPlace(card:getEffectiveId()) == sgs.Player_PlaceHand then
						sgs.ai_use_priority.FlameMapCard = 9
						--self.player:speak("有返回：" .. card:getLogName())
						use.card = sgs.Card_Parse("@FlameMapCard=" .. card:getEffectiveId() .. "&showforviewhas")
						return
					end
				else
					if dummy_use.card:isKindOf("EquipCard") and self:getSameEquip(card) then
						sgs.ai_use_priority.FlameMapCard = 9
						--self.player:speak("有返回：" .. self:getSameEquip(card):getLogName())
						use.card = sgs.Card_Parse("@FlameMapCard=" .. self:getSameEquip(card):getEffectiveId() .. "&showforviewhas")
						return
					end
				end
			end
		end
		if self.player:hasEquip() and not full then
			if self.player:getOffensiveHorse() then
				--self.player:speak("有返回：" .. self.player:getOffensiveHorse():getLogName())
				use.card = sgs.Card_Parse("@FlameMapCard=" .. self.player:getOffensiveHorse():getEffectiveId() .. "&showforviewhas")
				return
			end
			if self.player:getWeapon() then
				--self.player:speak("有返回：" .. self.player:getWeapon():getLogName())
				use.card = sgs.Card_Parse("@FlameMapCard=" .. self.player:getWeapon():getEffectiveId() .. "&showforviewhas")
				return
			end
			if not self:isWeak() then
				if self.player:getArmor() then
					--self.player:speak("有返回：" .. self.player:getArmor():getLogName())
					use.card = sgs.Card_Parse("@FlameMapCard=" .. self.player:getArmor():getEffectiveId() .. "&showforviewhas")
					return
				end
				if self.player:getDefensiveHorse() then
					--self.player:speak("有返回：" .. self.player:getDefensiveHorse():getLogName())
					use.card = sgs.Card_Parse("@FlameMapCard=" .. self.player:getDefensiveHorse():getEffectiveId() .. "&showforviewhas")
					return
				end
			end
		end
	end
end
sgs.ai_use_value.FlameMapCard = 10

--夜明珠
local Luminouspearl_skill = {}
Luminouspearl_skill.name = "Luminouspearl"
table.insert(sgs.ai_skills, Luminouspearl_skill)
Luminouspearl_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("ZhihengCard") and not self.player:hasShownSkill("zhiheng") and self.player:hasTreasure("Luminouspearl") then
		return sgs.Card_Parse("@ZhihengCard=.")
	end
end

sgs.ai_use_priority.Luminouspearl = 7
sgs.ai_keep_value.Luminouspearl = 4

--变更武将相关

function SmartAI:getGeneralValue(player, position)
	local general
	if position then
		general = player:getGeneral()
	else
		general = player:getGeneral2()
	end
	if general:objectName() == "anjiang" then
		if self.player:objectName() ~= player:objectName() then return 3 end
	else
		if position then
			general = player:getActualGeneral1()
		else
			general = player:getActualGeneral2()
		end
	end
	local ajust = 0
	for _, skill in sgs.qlist(general:getVisibleSkillList(true, position)) do
		if skill:getFrequency() == sgs.Skill_Limited and skill:getLimitMark() ~= "" and player:getMark(skill:getLimitMark()) == 0 then
            ajust = ajust - 1
		end
	end
	for name, value in pairs(sgs.general_value) do
		if general:objectName() == name then
			return value + ajust
		end
	end
	return 3
end

function SmartAI:needToTransform()
	local g1 = self.player:getActualGeneral1()
	local g2 = self.player:getActualGeneral2()
	local current_value = 0
	for name, value in pairs(sgs.general_pair_value) do
		if g1:objectName() .. "+" .. g2:objectName() == name or g2:objectName() .. "+" .. g1:objectName() == name then
			current_value = value
			break
		end
	end
	local oringin_g1 = 3
	local oringin_g2 = 3
	for name, value in pairs(sgs.general_value) do
		if g1:objectName() == name then oringin_g1 = value end
		if g2:objectName() == name then oringin_g2 = value end
	end
	if current_value == 0 then current_value = oringin_g1 + oringin_g2 end
	local g2_v = current_value - (oringin_g2 - self:getGeneralValue(self.player, false)) - oringin_g1
	return g2_v < 3
end

sgs.ai_skill_invoke.transform = function(self, data)
	return self:needToTransform()
end